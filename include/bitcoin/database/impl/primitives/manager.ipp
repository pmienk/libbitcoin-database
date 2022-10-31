/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_MANAGER_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::manager(storage& file) NOEXCEPT
  : file_(file)
{
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return position_to_link(file_.size());
}

TEMPLATE
bool CLASS::truncate(Link count) NOEXCEPT
{
    if (count.is_eof())
        return false;

    return file_.resize(link_to_position(count));
}

// allocated start and count are known.
// allocated (link-to-end) byte size is link_to_position(count).
TEMPLATE
Link CLASS::allocate(Link count) NOEXCEPT
{
    if (count.is_eof())
        return Link::eof;

    const auto position = file_.allocate(link_to_position(count));

    if (position == storage::eof)
        return Link::eof;

    return position_to_link(position);
}

// memory mutex guards against map reduction, so count() is known.
// count = count() - link, is elements from link to end.
// link-to-end byte size is link_to_position(count).
TEMPLATE
memory_ptr CLASS::get(Link value) const NOEXCEPT
{
    if (value.is_eof())
        return nullptr;

    return file_.get(link_to_position(value));
}

} // namespace database
} // namespace libbitcoin

#endif
