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
#ifndef LIBBITCOIN_DATABASE_QUERY_VALIDATE_IPP
#define LIBBITCOIN_DATABASE_QUERY_VALIDATE_IPP

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

// Validation (surrogate-keyed).
// ----------------------------------------------------------------------------

// protected
TEMPLATE
inline code CLASS::to_block_code(
    linkage<schema::code>::integer value) const NOEXCEPT
{
    switch (value)
    {
        // Block satisfies validation rules (prevouts unverified).
        case schema::block_state::valid:
            return error::block_valid;
        // Final: Block satisfies confirmation rules (prevouts).
        case schema::block_state::confirmable:
            return error::block_confirmable;
        // Final: Block does not satisfy validation/confirmation rules.
        case schema::block_state::unconfirmable:
            return error::block_unconfirmable;
        // Block has no recorded state, may be under checkpoint or milestone.
        default:
            return error::unknown_state;
    }
}

// protected
TEMPLATE
inline code CLASS::to_tx_code(
    linkage<schema::code>::integer value) const NOEXCEPT
{
    // Validation states are unrelated to confirmation rules.
    // All stored transactions are presumed valid in some possible context.
    // All states below are relevant only to the associated validation context.
    switch (value)
    {
        // Tx is valid in the case where standard prevouts are matched.
        case schema::tx_state::preconnected:
            return error::tx_preconnected;
        // Final: Tx is valid (passed check, accept, and connect).
        case schema::tx_state::connected:
            return error::tx_connected;
        // Final: Tx is not valid (failed check, accept, or connect).
        case schema::tx_state::disconnected:
            return error::tx_disconnected;
        // Tx has no recorded state, may be under checkpoint or milestone.
        default:
            return error::unknown_state;
    }
}

// protected
TEMPLATE
inline bool CLASS::is_sufficient(const context& current,
    const context& evaluated) const NOEXCEPT
{
    // Past evaluation at a lesser height and/or mtp is sufficient.
    return evaluated.flags == current.flags
        && evaluated.height <= current.height
        && evaluated.mtp <= current.mtp;
}

TEMPLATE
bool CLASS::get_timestamp(uint32_t& timestamp,
    const header_link& link) const NOEXCEPT
{
    table::header::get_timestamp header{};
    if (!store_.header.get(link, header))
        return false;

    timestamp = header.timestamp;
    return true;
}

TEMPLATE
bool CLASS::get_version(uint32_t& version,
    const header_link& link) const NOEXCEPT
{
    table::header::get_version header{};
    if (!store_.header.get(link, header))
        return false;

    version = header.version;
    return true;
}

TEMPLATE
bool CLASS::get_bits(uint32_t& bits, const header_link& link) const NOEXCEPT
{
    table::header::get_bits header{};
    if (!store_.header.get(link, header))
        return false;

    bits = header.bits;
    return true;
}

TEMPLATE
bool CLASS::get_context(context& ctx, const header_link& link) const NOEXCEPT
{
    table::header::record_context header{};
    if (!store_.header.get(link, header))
        return false;

    ctx = std::move(header.ctx);
    return true;
}

TEMPLATE
bool CLASS::get_context(system::chain::context& ctx,
    const header_link& link) const NOEXCEPT
{
    table::header::record_context header{};
    if (!store_.header.get(link, header))
        return false;

    // Context for block/header.check and header.accept are filled from
    // chain_state, not from the store.
    ctx =
    {
        header.ctx.flags,     // [block.check, block.accept & block.connect]
        {},                   // [block.check] timestamp
        header.ctx.mtp,       // [block.check, header.accept]
        header.ctx.height,    // [block.check & block.accept]
        {},                   // [header.accept] minimum_block_version
        {}                    // [header.accept] work_required
    };

    return true;
}

TEMPLATE
bool CLASS::get_work(uint256_t& work, const header_link& link) const NOEXCEPT
{
    uint32_t bits{};
    const auto result = get_bits(bits, link);
    work = header::proof(bits);
    return result;
}

////TEMPLATE
////bool CLASS::get_check_context(context& ctx, hash_digest& hash,
////    uint32_t& timestamp, const header_link& link) const NOEXCEPT
////{
////    table::header::get_check_context header{};
////    if (!store_.header.get(link, header))
////        return false;
////
////    hash = std::move(header.key);
////    ctx = std::move(header.ctx);
////    timestamp = header.timestamp;
////    return true;
////}

TEMPLATE
code CLASS::get_header_state(const header_link& link) const NOEXCEPT
{
    table::validated_bk::slab_get_code valid{};
    if (!store_.validated_bk.find(link, valid))
        return error::unvalidated;

    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_block_state(const header_link& link) const NOEXCEPT
{
    table::validated_bk::slab_get_code valid{};
    if (!store_.validated_bk.find(link, valid))
        return is_associated(link) ? error::unvalidated : error::unassociated;

    // Fees only valid if block_confirmable.
    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_block_state(uint64_t& fees,
    const header_link& link) const NOEXCEPT
{
    table::validated_bk::slab valid{};
    if (!store_.validated_bk.find(link, valid))
        return is_associated(link) ? error::unvalidated : error::unassociated;

    // Fees only valid if block_confirmable.
    fees = valid.fees;
    return to_block_code(valid.code);
}

TEMPLATE
code CLASS::get_tx_state(const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    auto it = store_.validated_tx.it(link);
    if (!it)
        return error::unvalidated;

    table::validated_tx::slab_get_code valid{};
    do
    {
        if (!store_.validated_tx.get(it, valid))
            return error::integrity;

        if (is_sufficient(ctx, valid.ctx))
            return to_tx_code(valid.code);
    }
    while (it.advance());
    return error::unvalidated;
}

TEMPLATE
code CLASS::get_tx_state(uint64_t& fee, size_t& sigops, const tx_link& link,
    const context& ctx) const NOEXCEPT
{
    auto it = store_.validated_tx.it(link);
    if (!it)
        return error::unvalidated;

    table::validated_tx::slab valid{};
    do
    {
        if (!store_.validated_tx.get(it, valid))
            return error::integrity;

        if (is_sufficient(ctx, valid.ctx))
        {
            fee = valid.fee;
            sigops = valid.sigops;
            return to_tx_code(valid.code);
        }
    }
    while (it.advance());
    return error::unvalidated;
}

TEMPLATE
bool CLASS::set_block_valid(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::valid,
        0 // fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_block_confirmable(const header_link& link,
    uint64_t fees) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::confirmable,
        fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_block_unconfirmable(const header_link& link) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_bk.put(link, table::validated_bk::slab
    {
        {},
        schema::block_state::unconfirmable,
        0 // fees
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_preconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::preconnected,
        0, // fee
        0  // sigops
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_disconnected(const tx_link& link,
    const context& ctx) NOEXCEPT
{
    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::disconnected,
        0, // fee
        0  // sigops
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_tx_connected(const tx_link& link, const context& ctx,
    uint64_t fee, size_t sigops) NOEXCEPT
{
    using sigs = linkage<schema::sigops>;
    BC_ASSERT(sigops<system::power2<sigs::integer>(to_bits(sigs::size)));

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return store_.validated_tx.put(link, table::validated_tx::slab
    {
        {},
        ctx,
        schema::tx_state::connected,
        fee,
        system::possible_narrow_cast<sigs::integer>(sigops)
    });
    // ========================================================================
}

TEMPLATE
bool CLASS::set_txs_connected(const header_link& link) NOEXCEPT
{
    context ctx{};
    if (!get_context(ctx, link))
        return false;

    const auto txs = to_transactions(link);
    if (txs.empty())
        return false;

    // FOR PERFORMANCE EVALUATION ONLY.
    constexpr uint64_t fee = 99;
    constexpr size_t sigops = 42;
    using sigs = linkage<schema::sigops>;

    // ========================================================================
    const auto scope = store_.get_transactor();

    // Clean single allocation failure (e.g. disk full).
    return std_all_of(bc::seq, txs.begin(), txs.end(),
        [&](const tx_link& fk) NOEXCEPT
        {
            return store_.validated_tx.put(fk, table::validated_tx::slab
            {
                {},
                ctx,
                schema::tx_state::connected,
                fee,
                system::possible_narrow_cast<sigs::integer>(sigops)
            });
        });
    // ========================================================================
}

} // namespace database
} // namespace libbitcoin

#endif
