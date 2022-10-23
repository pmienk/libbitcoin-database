/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_MEMORY_STORAGE_HPP
#define LIBBITCOIN_DATABASE_MEMORY_STORAGE_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {

/// Mapped memory abstraction of a file.
class BCD_API storage
{
public:
    static constexpr auto eof = system::bit_all<size_t>;

    /// The current capacity of the memory map (zero if unmapped).
    virtual size_t capacity() const NOEXCEPT = 0;

    /// The current logical size of the memory map (zero if closed).
    virtual size_t size() const NOEXCEPT = 0;

    /// Set logical size to specified (false if size exceeds capacity).
    virtual bool resize(size_t size) NOEXCEPT = 0;

    /// Allocate bytes and return offset to first allocated (or eof).
    virtual size_t allocate(size_t chunk) NOEXCEPT = 0;

    /// Get r/w access to start/offset of memory map (or null).
    virtual memory_ptr get(size_t offset=zero) NOEXCEPT = 0;
};

} // namespace database
} // namespace libbitcoin

#endif
