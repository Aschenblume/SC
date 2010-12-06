//  nova server, buffer manager class
//  Copyright (C) 2009 Tim Blechmann
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef SERVER_BUFFER_MANAGER_HPP
#define SERVER_BUFFER_MANAGER_HPP

#include <algorithm>
#include <cassert>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "simd/simd_memory.hpp"
#include "utilities/malloc_aligned.hpp"


namespace nova
{

struct buffer_wrapper
{
    typedef std::size_t size_t;
    typedef float sample_t;

    buffer_wrapper(void):
        data(0), frames_(0), channels_(0), sample_rate_(0)
    {}

    ~buffer_wrapper(void)
    {
        if (data)
            free_aligned(data);
    }

    void allocate(size_t frames, uint channels);

    void free(void)
    {
        if (data) {
            free_aligned(data);
            data = 0;
            frames_ = 0;
            channels_ = 0;
        }
    }

    void zero(void)
    {
        assert (data);
        zerovec(data, frames_);
    }

    /** set sample for specific indices */
    template <typename float_type>
    void set_samples(uint count, const size_t * indices, const float_type * values)
    {
        for (int i = 0; i != count; ++i) {
            size_t index = indices[i];
            if (index < frames_) {
                sample_t value = values[i];
                data[index] = value;
            }
        }
    }

    /** set continuous samples starting at position */
    template <typename float_type>
    void set_samples(size_t position, uint count, const float_type * values)
    {
        sample_t * base = data + position;
        size_t avail = frames_ - position;
        count = std::min(size_t(count), avail);

        for (int i = 0; i != count; ++i)
            base[i] = sample_t(values[i]);
    }

    /** set continuous samples starting at position to a single value */
    template <typename float_type>
    void fill_samples(size_t position, uint count, const float_type value)
    {
        sample_t * base = data + position;
        size_t avail = frames_ - position;
        count = std::min(size_t(count), avail);

        sample_t val = sample_t(value);
        for (int i = 0; i != count; ++i)
            base[i] = val;
    }

    /* @{ */
    void read_file(std::string const & file, size_t start_frame, size_t frames)
    {
        read_file(file.c_str(), start_frame, frames);
    }

    void read_file(const char * file, size_t start_frame, size_t frames);

    void read_file_channels(std::string const & file, size_t start_frame, size_t frames,
                            uint channel_count, uint * channels)
    {
        read_file_channels(file.c_str(), start_frame, frames, channel_count, channels);
    }

    void read_file_channels(const char * file, size_t start_frame, size_t frames,
                            uint channel_count, uint * channels);
    /* @} */

    void write_file(const char * file, const char * header_format, const char * sample_format,
                    size_t start_frame, size_t frames);

    sample_t * data;
    size_t frames_;
    uint channels_;
    int sample_rate_;
};

class buffer_manager
{
public:
    buffer_manager(uint max_buffers):
        buffers(max_buffers, buffer_wrapper())
    {}

    void check_buffer_unused(int index)
    {
        if (buffers[index].data != NULL)
            throw std::runtime_error("buffer already in use");
    }

    void check_buffer_in_use(int index)
    {
        if (buffers[index].data == NULL)
            throw std::runtime_error("buffer is not in use");
    }

    void allocate_buffer(int index, uint frames, uint channels)
    {
        check_buffer_unused(index);
        buffers[index].allocate(frames, channels);
    }

    void read_buffer_allocate(int index, const char * file, size_t start_frame, size_t frames)
    {
        check_buffer_unused(index);
        buffers[index].read_file(file, start_frame, frames);
    }

    void read_buffer_channels_allocate(int index, const char * file, size_t start_frame, size_t frames,
                                       uint channel_count, uint * channels)
    {
        check_buffer_unused(index);
        buffers[index].read_file_channels(file, start_frame, frames, channel_count, channels);
    }

    void free_buffer(int index)
    {
        check_buffer_in_use(index);
        buffers[index].free();
    }

    void zero_buffer(int index)
    {
        check_buffer_in_use(index);
        buffers[index].zero();
    }

    template <typename float_type>
    void set_samples(int index, uint count, const size_t * indices, const float_type * values)
    {
        check_buffer_in_use(index);
        buffers[index].set_samples(count, indices, values);
    }

    template <typename float_type>
    void set_samples(int index, size_t position, uint count, const float_type * values)
    {
        check_buffer_in_use(index);
        buffers[index].set_samples(count, position, count, values);
    }

    template <typename float_type>
    void fill_samples(int index, size_t position, uint count, float_type value)
    {
        check_buffer_in_use(index);
        buffers[index].fill_samples(position, count, value);
    }

    void write_buffer(int index, const char * file, const char * header_format, const char * sample_format,
                      size_t start_frame, size_t frames)
    {
        check_buffer_in_use(index);
        buffers[index].write_file(file, header_format, sample_format, start_frame, frames);
    }

private:
    std::vector<buffer_wrapper> buffers;
};

} /* namespace nova */

#undef foreach
#endif /* SERVER_BUFFER_MANAGER_HPP */
