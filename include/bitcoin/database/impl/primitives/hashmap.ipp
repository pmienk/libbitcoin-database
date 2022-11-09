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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap(storage& header, storage& body, const link& buckets) NOEXCEPT
  : header_(header, buckets), body_(body)
{
}

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    return header_.create() && verify();
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    link count{};
    return header_.verify() && header_.get_body_count(count) &&
        count == body_.count();
}

TEMPLATE
bool CLASS::exists(const key& key) const NOEXCEPT
{
    return !first(key).is_terminal();
}

TEMPLATE
Record CLASS::get(const key& key) const NOEXCEPT
{
    return { first(key) };
}

TEMPLATE
Record CLASS::get(const link& link) const NOEXCEPT
{
    return { at(link) };
}

TEMPLATE
Iterator CLASS::iterator(const key& key) const NOEXCEPT
{
    return { body_, header_.top(key), key };
}

TEMPLATE
bool CLASS::insert(const key& key, const Record& record) NOEXCEPT
{
    // record.size() is slab/byte or record allocation.
    return record.to_data(push(key, record.size()));
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
typename CLASS::link CLASS::first(const key& key) const NOEXCEPT
{
    return iterator(key).self();
}

TEMPLATE
reader_ptr CLASS::at(const link& record) const NOEXCEPT
{
    if (record.is_terminal())
        return {};

    const auto ptr = body_.get(record);
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);
    source->skip_bytes(link_size);
    if constexpr (!slab) { source->set_limit(payload_size); }
    return source;
}

TEMPLATE
reader_ptr CLASS::find(const key& key) const NOEXCEPT
{
    const auto record = first(key);
    if (record.is_terminal())
        return {};

    const auto source = at(record);
    if (!source)
        return {};

    source->skip_bytes(key_size);
    return source;
}

TEMPLATE
writer_ptr CLASS::push(const key& key, const link& size) NOEXCEPT
{
    BC_ASSERT(!size.is_terminal());
    BC_ASSERT(!system::is_multiply_overflow<size_t>(size, payload_size));

    const auto item = body_.allocate(size);
    if (item.is_terminal())
        return {};

    const auto ptr = body_.get(item);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<writer>(ptr);
    const auto index = header_.index(key);

    sink->set_finalizer([this, item, index, ptr]() NOEXCEPT
    {
        BC_PUSH_WARNING(NO_REINTERPRET_CAST)
        using namespace system;
        auto& next = unsafe_array_cast<uint8_t, link::size>(ptr->begin());
        return header_.push(item, next, index);
        BC_POP_WARNING()
    });

    if constexpr (slab) { sink->set_limit(size); }
    sink->skip_bytes(link_size);
    if constexpr (!slab) { sink->set_limit(size * payload_size); }
    sink->write_bytes(key);
    return sink;
}

} // namespace database
} // namespace libbitcoin

#endif
