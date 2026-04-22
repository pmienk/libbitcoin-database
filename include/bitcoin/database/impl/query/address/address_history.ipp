/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP
#define LIBBITCOIN_DATABASE_QUERY_ADDRESS_HISTORY_IPP

#include <atomic>
#include <algorithm>
#include <ranges>
#include <utility>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Address history
// ----------------------------------------------------------------------------
// Canonically-sorted/deduped address history.
// root txs (height:zero) sorted before transitive (height:max) txs.
// tied-height transactions sorted by base16 txid (not converted).
// All confirmed txs are root, unconfirmed may or may not be root.

// server/electrum
TEMPLATE
code CLASS::get_unconfirmed_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    address_link cursor{};
    return get_unconfirmed_history(cancel, cursor, out, key, max_size_t, turbo);
}

// server/electrum
TEMPLATE
code CLASS::get_unconfirmed_history(const stopper& cancel,
    address_link& cursor, histories& out, const hash_digest& key,
    size_t limit, bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, cursor, txs, key, limit))
        return ec;

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = get_tx_unconfirmed_history(link);
            if (!out.confirmed() && !out.valid()) fail = true;
            return out;
        });
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    address_link cursor{};
    return get_confirmed_history(cancel, cursor, out, key, max_size_t, turbo);
}

// ununsed
TEMPLATE
code CLASS::get_confirmed_history(const stopper& cancel,
    address_link& cursor, histories& out, const hash_digest& key,
    size_t limit, bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, cursor, txs, key, limit))
        return ec;

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = get_tx_confirmed_history(link);
            if (out.confirmed() && !out.valid()) fail = true;
            return out;
        });
}

// ununsed
TEMPLATE
code CLASS::get_history(const stopper& cancel, histories& out,
    const hash_digest& key, bool turbo) const NOEXCEPT
{
    address_link cursor{};
    return get_history(cancel, cursor, out, key, max_size_t, turbo);
}

// server/electrum
TEMPLATE
code CLASS::get_history(const stopper& cancel, address_link& cursor,
    histories& out, const hash_digest& key, size_t limit,
    bool turbo) const NOEXCEPT
{
    tx_links txs{};
    if (const auto ec = get_address_txs(cancel, cursor, txs, key, limit))
        return ec;

    out.clear();
    out.resize(txs.size());
    return parallel_history_transform(cancel, turbo, out, txs,
        [this](const tx_link& link, auto& cancel, auto& fail) NOEXCEPT
        {
            if (cancel || fail) return history{};
            const auto out = get_tx_history(link);
            if (!out.valid()) fail = true;
            return out;
        });
}

// History queries.
// ----------------------------------------------------------------------------

TEMPLATE
history CLASS::get_tx_history(const tx_link& link) const NOEXCEPT
{
    return get_tx_history(get_tx_key(link), link);
}

// protected
TEMPLATE
history CLASS::get_tx_history(hash_digest&& key,
    const tx_link& link) const NOEXCEPT
{
    // history is invalid in default construction.
    if (link.is_terminal())
        return {};

    // Electrum uses fees only on unconfirmed.
    auto fee = history::missing_prevout;
    auto height = history::unrooted_height;
    auto position = history::unconfirmed_position;

    const auto block = find_strong(link);
    if (is_confirmed_block(block))
    {
        if (!get_height(height, block) ||
            !get_tx_position(position, link, block))
            return {};
    }
    else
    {
        if (!get_tx_fee(fee, link))
            fee = history::missing_prevout;

        if (is_confirmed_all_prevouts(link))
            height = history::rooted_height;
    }

    return { { std::move(key), height }, fee, position };
}

TEMPLATE
history CLASS::get_tx_confirmed_history(const tx_link& link) const NOEXCEPT
{
    return get_tx_confirmed_history(get_tx_key(link), link);
}

// protected
TEMPLATE
history CLASS::get_tx_confirmed_history(hash_digest&& key,
    const tx_link& link) const NOEXCEPT
{
    // history is invalid in default construction.
    if (link.is_terminal())
        return { .position = zero };

    const auto block = find_strong(link);

    // Returns invalid (filtered) but also !confirmed().
    if (!is_confirmed_block(block))
        return { .position = history::unconfirmed_position };

    size_t height{}, position{};
    if (!get_height(height, block) ||
        !get_tx_position(position, link, block))
        return { .position = zero };

    // Electrum uses fees only on unconfirmed (expensive).
    return { { std::move(key), height }, history::missing_prevout, position };
}

TEMPLATE
history CLASS::get_tx_unconfirmed_history(const tx_link& link) const NOEXCEPT
{
    return get_tx_unconfirmed_history(get_tx_key(link), link);
}

// protected
TEMPLATE
history CLASS::get_tx_unconfirmed_history(hash_digest&& key,
    const tx_link& link) const NOEXCEPT
{
    // history is invalid in default construction.
    if (link.is_terminal())
        return { .position = history::unconfirmed_position };

    // Returns invalid (filtered) but also confirmed().
    if (is_confirmed_block(find_strong(link)))
        return { .position = zero };

    uint64_t fee{};
    if (!get_tx_fee(fee, link))
        fee = history::missing_prevout;

    const auto height = is_confirmed_all_prevouts(link) ?
        history::rooted_height : history::unrooted_height;

    return { { std::move(key), height }, fee, history::unconfirmed_position };
}

// server/electrum
TEMPLATE
histories CLASS::get_spenders_history(
    const system::chain::point& prevout) const NOEXCEPT
{
    const auto ins = to_spenders(prevout);
    histories out(ins.size());
    for (const auto& in: std::views::reverse(ins))
        out.push_back(get_tx_history(to_input_tx(in)));

    history::filter_sort_and_dedup(out);
    return out;
}

TEMPLATE
histories CLASS::get_spenders_history(const hash_digest& key,
    uint32_t index) const NOEXCEPT
{
    return get_spenders_history({ key , index });
}

// utilities
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_address_txs(const stopper& cancel, address_link& cursor,
    tx_links& out, const hash_digest& key, size_t limit) const NOEXCEPT
{
    output_links links{};
    if (const auto ec = to_address_outputs(cancel, cursor, links, key, limit))
        return ec;

    return to_touched_txs(cancel, out, links);
}

// private/static
TEMPLATE
template <typename Functor>
code CLASS::parallel_history_transform(const stopper& cancel, bool turbo,
    histories& out, const tx_links& txs, Functor&& functor) NOEXCEPT
{
    const auto policy = poolstl::execution::par_if(turbo);
    stopper fail{};

    out.clear();
    out.resize(txs.size());
    std::transform(policy, txs.cbegin(), txs.cend(), out.begin(),
        [&functor, &cancel, &fail](const auto& tx) NOEXCEPT
        {
            return functor(tx, cancel, fail);
        });

    if (fail)
        return error::integrity;

    if (cancel)
        return error::query_canceled;

    history::filter_sort_and_dedup(out);
    return error::success;
}

} // namespace database
} // namespace libbitcoin

#endif
