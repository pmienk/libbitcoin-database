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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ELEMENT_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
INLINE CLASS::iterator(const memory_ptr& data, const Link& start,
    const Key& key) NOEXCEPT
  : memory_(data), key_(key), link_(start)
{
    if (!is_match())
        advance();
}

TEMPLATE
INLINE bool CLASS::advance() NOEXCEPT
{
    while (!link_.is_terminal())
    {
        link_ = get_next();
        if (is_match())
            return true;
    }

    return false;
}

TEMPLATE
INLINE const Link& CLASS::self() const NOEXCEPT
{
    return link_;
}

TEMPLATE
INLINE const memory_ptr& CLASS::get() const NOEXCEPT
{
    return memory_;
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
INLINE bool CLASS::is_match() const NOEXCEPT
{
    using namespace system;
    if (link_.is_terminal() || !memory_)
        return false;

    BC_ASSERT(!is_add_overflow(link_to_position(link_), Link::size));
    auto link = memory_->offset(link_to_position(link_) + Link::size);
    return !is_null(link) && std::equal(key_.begin(), key_.end(), link);
}

TEMPLATE
INLINE Link CLASS::get_next() const NOEXCEPT
{
    if (link_.is_terminal() || !memory_)
        return {};

    const auto link = memory_->offset(link_to_position(link_));
    if (is_null(link))
        return {};

    return { system::unsafe_array_cast<uint8_t, Link::size>(link) };
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(link.value);

    if constexpr (is_slab)
    {
        // Slab implies link/key incorporated into size.
        return value;
    }
    else
    {
        // Record implies link/key independent of Size.
        constexpr auto element_size = Link::size + array_count<Key> + Size;
        BC_ASSERT(!system::is_multiply_overflow(value, element_size));
        return value * element_size;
    }
}

} // namespace database
} // namespace libbitcoin

#endif
