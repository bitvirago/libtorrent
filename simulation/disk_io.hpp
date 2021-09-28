/*

Copyright (c) 2021, Arvid Norberg
All rights reserved.

You may use, distribute and modify this code under the terms of the BSD license,
see LICENSE file.
*/

#include "libtorrent/disk_interface.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/file_storage.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/aux_/merkle.hpp"
#include "libtorrent/io_context.hpp"
#include "test_utils.hpp"

#include <array>
#include <limits>

std::array<char, 0x4000> generate_block_fill(lt::piece_index_t const p, int const block);
lt::sha1_hash generate_hash1(lt::piece_index_t const p, lt::file_storage const& fs);
lt::sha1_hash generate_hash2(lt::piece_index_t p, lt::file_storage const& fs
	, lt::span<lt::sha256_hash> const hashes);
lt::sha256_hash generate_block_hash(lt::piece_index_t p, int offset);
void generate_block(char* b, lt::peer_request const& r);
std::shared_ptr<lt::torrent_info> create_test_torrent(int piece_size
	, int num_pieces, lt::create_flags_t flags, int num_files = 1);
lt::add_torrent_params create_test_torrent(
	int num_pieces, lt::create_flags_t flags, int blocks_per_piece, int num_files = 1);

struct test_disk
{
	test_disk set_seed(bool const s = true) const
	{
		auto ret = *this;
		ret.seed = s;
		return ret;
	}
	test_disk set_space_left(int const left) const
	{
		auto ret = *this;
		ret.space_left = left;
		return ret;
	}
	test_disk set_recover_full_disk() const
	{
		auto ret = *this;
		ret.recover_full_disk = true;
		return ret;
	}
	test_disk send_corrupt_data(int const blocks) const
	{
		auto ret = *this;
		ret.corrupt_data_in = blocks;
		return ret;
	}

	// the number of blocks/write jobs in the queue before we exceed the write
	// queue size. Once the level drops below the low watermark, we allow writes
	// again
	int high_watermark = 50;
	int low_watermark = 40;

	std::unique_ptr<lt::disk_interface> operator()(
		lt::io_context& ioc, lt::settings_interface const&, lt::counters&);

	// seek time in fron of every read and write
	lt::time_duration seek_time = lt::milliseconds(10);

	// hash time per block
	lt::time_duration hash_time = lt::microseconds(15);

	// write time per block
	lt::time_duration write_time = lt::microseconds(2);

	// read time per block
	lt::time_duration read_time = lt::microseconds(1);

	// we have all files, with valid data
	bool seed = false;

	// after having failed with disk-full error, reset space_left to int_max
	bool recover_full_disk = false;

	// after sending this many blocks, send corrupt data
	int corrupt_data_in = std::numeric_limits<int>::max();

	// after having written this many bytes, fail with disk-full
	int space_left = std::numeric_limits<int>::max();
};

