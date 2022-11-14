/**
/// Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
 *
/// This file is part of libbitcoin.
 *
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
 *
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
 *
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAY_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>
#include <bitcoin/database/memory/memory.hpp>

namespace libbitcoin {
namespace database {
    
/// Caution: reader/writer hold body remap lock until disposed.
/// These handles should be used for serialization and immediately disposed.
template <typename Link, size_t Size>
class arraymap
{
public:
    arraymap(storage& body) NOEXCEPT;

    /// Query interface.

    /// RECORD.FROM_DATA OBTAINS SHARED LOCK ON STORAGE REMAP.
    template <typename Record, if_equal<Record::size, Size> = true>
    Record get(const Link& link) const NOEXCEPT;

    /// RECORD.TO_DATA OBTAINS SHARED LOCK ON STORAGE REMAP.
    template <typename Record, if_equal<Record::size, Size> = true>
    bool put(const Record& record) NOEXCEPT;

protected:
    /// Reader positioned at data.
    /// READER HOLDS SHARED LOCK ON STORAGE REMAP.
    reader_ptr at(const Link& link) const NOEXCEPT;

    /// Reader positioned at data.
    /// WRITER HOLDS SHARED LOCK ON STORAGE REMAP.
    writer_ptr push(const Link& size=one) NOEXCEPT;

private:
    static constexpr auto is_slab = (Size == max_size_t);
    static constexpr size_t link_to_position(const Link& link) NOEXCEPT;

    // Thread safe.
    storage& body_;
};

// Use to standardize arraymap declarations, assumes "record" within namespace.
#define RECORDMAP arraymap<linkage<record::pk>, record::size>
#define SLABMAP arraymap<linkage<slab::pk>, slab::size>

} // namespace database
} // namespace libbitcoin

#define TEMPLATE template <typename Link, size_t Size>
#define CLASS arraymap<Link, Size>

#include <bitcoin/database/impl/primitives/arraymap.ipp>

#undef CLASS
#undef TEMPLATE

#endif
