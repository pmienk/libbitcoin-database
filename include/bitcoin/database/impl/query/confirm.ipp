/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP
#define LIBBITCOIN_DATABASE_QUERY_CONFIRM_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/error.hpp>

namespace libbitcoin {
namespace database {

// Block status (surrogate-keyed).
// ----------------------------------------------------------------------------
// Not for use in validatation (2 additional gets) or confirmation (height).

// protected
TEMPLATE
height_link CLASS::get_height(const header_link& link) const NOEXCEPT
{
    table::header::get_height header{};
    if (!store_.header.get(link, header))
        return {};

    return header.height;
}

// protected/get_confirmed_balance(address)
TEMPLATE
bool CLASS::is_confirmed_unspent(const output_link& link) const NOEXCEPT
{
    return is_confirmed_output(link) && !is_spent_output(link);
}

TEMPLATE
bool CLASS::is_candidate_block(const header_link& link) const NOEXCEPT
{
    // The block is candidate (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record candidate{};
    return store_.candidate.get(height, candidate) &&
        (candidate.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_block(const header_link& link) const NOEXCEPT
{
    // The block is confirmed (by height).
    const auto height = get_height(link);
    if (height.is_terminal())
        return false;

    table::height::record confirmed{};
    return store_.confirmed.get(height, confirmed) &&
        (confirmed.header_fk == link);
}

TEMPLATE
bool CLASS::is_confirmed_tx(const tx_link& link) const NOEXCEPT
{
    // The tx is strong *and* its block is confirmed (by height).
    const auto fk = to_block(link);
    return !fk.is_terminal() && is_confirmed_block(fk);
}

TEMPLATE
bool CLASS::is_confirmed_input(const spend_link& link) const NOEXCEPT
{
    // The spend.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_spend_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_confirmed_output(const output_link& link) const NOEXCEPT
{
    // The output.tx is strong *and* its block is confirmed (by height).
    const auto fk = to_output_tx(link);
    return !fk.is_terminal() && is_confirmed_tx(fk);
}

TEMPLATE
bool CLASS::is_spent_output(const output_link& link) const NOEXCEPT
{
    // The spender is strong *and* its block is confirmed (by height).
    const auto ins = to_spenders(link);
    return std::any_of(ins.begin(), ins.end(), [&](const auto& in) NOEXCEPT
    {
        return is_confirmed_input(in);
    });
}

// Confirmation.
// ----------------------------------------------------------------------------
// Block confirmed by height is not used for confirmation (just strong tx).

// unused
TEMPLATE
bool CLASS::is_spent(const spend_link& link) const NOEXCEPT
{
    table::spend::get_prevout_parent spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return false;

    return spent_prevout(spend.prevout(), spend.parent_fk);
}

// unused
TEMPLATE
bool CLASS::is_strong(const spend_link& link) const NOEXCEPT
{
    return !to_block(to_spend_tx(link)).is_terminal();
}

// unused
TEMPLATE
bool CLASS::is_mature(const spend_link& link, size_t height) const NOEXCEPT
{
    table::spend::get_point spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return true;

    return mature_prevout(spend.point_fk, height) == error::success;
}

// protected (only for is_mature/unused)
TEMPLATE
error::error_t CLASS::mature_prevout(const point_link& link,
    size_t height) const NOEXCEPT
{
    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::integrity;

    // to_block assures confirmation by strong_tx traversal so this must remain
    // prior to is_coinbase in execution order, despite the lack of dependency.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    if (!is_coinbase(tx_fk))
        return error::success;

    const auto prevout_height = get_height(header_fk);
    if (prevout_height.is_terminal())
        return error::integrity;

    if (!transaction::is_coinbase_mature(prevout_height, height))
        return error::coinbase_maturity;

    return error::success;
}

// unused
TEMPLATE
bool CLASS::is_locked(const spend_link& link, uint32_t sequence,
    const context& ctx) const NOEXCEPT
{
    table::spend::get_point spend{};
    if (!store_.spend.get(link, spend))
        return false;

    if (spend.is_null())
        return true;

    return locked_prevout(spend.point_fk, sequence, ctx) == error::success;
}

// protected (only for is_locked/unused)
TEMPLATE
error::error_t CLASS::locked_prevout(const point_link& link, uint32_t sequence,
    const context& ctx) const NOEXCEPT
{
    if (!ctx.is_enabled(system::chain::flags::bip68_rule))
        return error::success;

    // Get hash from point, search for prevout tx and get its link.
    const auto tx_fk = to_tx(get_point_key(link));
    if (tx_fk.is_terminal())
        return error::missing_previous_output;

    // to_block assures confirmation by strong_tx traversal.
    const auto header_fk = to_block(tx_fk);
    if (header_fk.is_terminal())
        return error::unconfirmed_spend;

    context prevout_ctx{};
    if (!get_context(prevout_ctx, header_fk))
        return error::integrity;

    if (input::is_locked(sequence, ctx.height, ctx.mtp, prevout_ctx.height,
        prevout_ctx.mtp))
        return error::relative_time_locked;

    return error::success;
}

// protected
TEMPLATE
inline error::error_t CLASS::spent_prevout(tx_link link,
    index index) const NOEXCEPT
{
    return spent_prevout(table::spend::compose(link, index),
        tx_link::terminal);
}

// protected
TEMPLATE
inline error::error_t CLASS::spent_prevout(const foreign_point& point,
    const tx_link& self) const NOEXCEPT
{
    auto it = store_.spend.it(point);
    if (it.self().is_terminal())
        return error::success;

    table::spend::get_parent spend{};
    do
    {
        if (!store_.spend.get(it.self(), spend))
            return error::integrity;

        // Skip current spend, which is the only one if not double spent.
        if (spend.parent_fk == self)
            continue;

        // If strong spender exists then prevout is confirmed double spent.
        if (!to_block(spend.parent_fk).is_terminal())
            return error::confirmed_double_spend;
    }
    while (it.advance());
    return error::success;
}

// protected
TEMPLATE
inline error::error_t CLASS::unspendable_prevout(const point_link& link,
    uint32_t sequence, uint32_t version, const context& ctx) const NOEXCEPT
{
    const auto strong = to_strong(get_point_key(link));
    if (strong.block.is_terminal())
        return strong.tx.is_terminal() ? error::missing_previous_output :
            error::unconfirmed_spend;

    context out{};
    if (!get_context(out, strong.block))
        return error::integrity;

    if (is_coinbase(strong.tx) &&
        !transaction::is_coinbase_mature(out.height, ctx.height))
        return error::coinbase_maturity;

    if (ctx.is_enabled(system::chain::flags::bip68_rule) &&
        (version >= system::chain::relative_locktime_min_version) &&
        input::is_locked(sequence, ctx.height, ctx.mtp, out.height, out.mtp))
        return error::relative_time_locked;

    return error::success;
}

TEMPLATE
inline error::error_t CLASS::unspent_duplicates(const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    if (!ctx.is_enabled(system::chain::flags::bip30_rule))
        return error::success;

    const auto coinbases = to_strongs(get_tx_key(link));
    if (coinbases.empty())
        return error::integrity;

    if (is_one(coinbases.size()))
        return error::success;

    // bip30: all (but self) must be confirmed spent or dup invalid (cb only).
    size_t unspent{};
    for (const auto& coinbase: coinbases)
        for (index out{}; out < output_count(coinbase.tx); ++out)
            if (!spent_prevout(coinbase.tx, out) && is_one(unspent++))
                return error::unspent_coinbase_collision;

    return is_zero(unspent) ? error::integrity : error::success;
}

TEMPLATE
code CLASS::block_confirmable(const header_link& link) const NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return error::integrity;

    const auto txs = to_txs(link);
    if (txs.empty())
        return error::success;

    code ec{};
    if ((ec = unspent_duplicates(txs.front(), ctx)))
        return ec;

    uint32_t version{};
    table::spend::get_prevout_sequence spend{};
    for (auto tx = std::next(txs.begin()); tx != txs.end(); ++tx)
    {
        for (const auto& spend_fk: to_tx_spends(version, *tx))
        {
            if (!store_.spend.get(spend_fk, spend))
                return error::integrity;

            if ((ec = unspendable_prevout(spend.point_fk, spend.sequence,
                version, ctx)))
                return ec;

            if ((ec = spent_prevout(spend.prevout(), *tx)))
                return ec;
        }
    }

    return error::success;
}

TEMPLATE
bool CLASS::set_strong(const header_link& link) NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, link, true };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full), block not confirmed.
    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_unstrong(const header_link& link) NOEXCEPT
{
    const auto txs = to_txs(link);
    if (txs.empty())
        return false;

    const table::strong_tx::record strong{ {}, link, false };

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean allocation failure (e.g. disk full), block not unconfirmed.
    return std::all_of(txs.begin(), txs.end(), [&](const tx_link& fk) NOEXCEPT
    {
        return store_.strong_tx.put(fk, strong);
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::initialize(const block& genesis) NOEXCEPT
{
    BC_ASSERT(!is_initialized());
    BC_ASSERT(genesis.transactions_ptr()->size() == one);

    // ========================================================================
    const auto scope = store_.get_transactor();

    const context ctx{};
    if (!set(genesis, ctx))
        return false;

    constexpr auto fees = 0u;
    constexpr auto sigops = 0u;
    const auto link = to_header(genesis.hash());

    // Unsafe for allocation failure, but only used in store creation.
    return set_strong(header_link{ 0 })
        && set_tx_connected(tx_link{ 0 }, ctx, fees, sigops) // tx valid.
        && set_block_confirmable(link, fees) // rename, block valid step.
        && push_candidate(link)
        && push_confirmed(link);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_candidate(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record candidate{ {}, link };
    return store_.candidate.put(candidate);
    // ========================================================================
}

TEMPLATE
bool CLASS::push_confirmed(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    const table::height::record confirmed{ {}, link };
    return store_.confirmed.put(confirmed);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_candidate() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_candidate());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.candidate.truncate(top);
    // ========================================================================
}

TEMPLATE
bool CLASS::pop_confirmed() NOEXCEPT
{
    using ix = table::transaction::ix::integer;
    const auto top = system::possible_narrow_cast<ix>(get_top_confirmed());
    if (is_zero(top))
        return false;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.confirmed.truncate(top);
    // ========================================================================
}

////// TEMP: testing cached values for confirmation.
////struct cached_point
////{
////    // input (under validation)
////    foreign_point key; // double-spendness
////    uint32_t parent;   // input.parent
////    uint32_t sequence; // bip68
////
////    // input->prevout
////    uint32_t tx;       // confirmedness
////    uint32_t height;   // bip68/maturity
////    uint32_t mtp;      // bip68
////    bool coinbase;     // maturity
////};
////// coinbase aligns at 4 bytes on msvc x64.
////////static_assert(sizeof(cached_point) == 32u);
////using cached_points = std_vector<cached_point>;
////bool create_cached_points(cached_points& out,
////    const header_link& link) const NOEXCEPT;
////TEMPLATE
////code CLASS::point_confirmable(const cached_point& point) const NOEXCEPT
////{
////    code ec{ error::success };
////
////    // height is always needed for coinbase (maturity).
////    // sequence value tells which is needed, height or mtp (bip68).
////    // but both may apply, so need to allow for two (no flags at prevout).
////    const context ctx{ 0, point.height, point.mtp };
////    if ((ec = unspendable_prevout(point.tx, point.coinbase, point.sequence, ctx)))
////        return ec;
////
////    // may only be strong-spent by self (and must be but is not checked).
////    if (spent_prevout(point.key, point.parent))
////        return error::confirmed_double_spend;
////
////    return ec;
////}
////
////TEMPLATE
////bool CLASS::create_cached_points(cached_points& out,
////    const header_link& link) const NOEXCEPT
////{
////    context ctx{};
////    if (!get_context(ctx, link))
////        return false;
////
////    table::input::get_prevout_sequence input{};
////    for (const auto& in: to_non_coinbase_inputs(link))
////    {
////        if (!store_.input.get(in, input))
////            return false;
////    
////        out.emplace_back
////        (
////            // input (under validation)
////            input.prevout(),
////            input.parent_fk,
////            input.sequence,
////
////            // input->prevout
////            to_tx(get_point_key(input.point_fk)),
////            system::possible_narrow_cast<uint32_t>(ctx.height),
////            ctx.mtp,
////            is_coinbase(spent_fk)
////        );
////    }
////
////    return true;
////}

} // namespace database
} // namespace libbitcoin

#endif
