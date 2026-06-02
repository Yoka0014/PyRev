#pragma once

#include <cstdint>

#include "constant.h"
#include "mobility.h"
#include "flip.h"

#include "utils/bitmanip.h"
#include "utils/array.h"
#include "utils/unroller.h"
#include "utils/crc32.h"

namespace reversi
{
    struct __Bitboard
    {
    private:
		static uint64_t delta_swap(uint64_t x, uint64_t mask, int delta)
		{
			auto t = (x ^ (x >> delta)) & mask;
			return x ^ t ^ (t << delta);
		}

		static uint64_t mirror_horizontal(uint64_t bb)
		{
			bb = delta_swap(bb, 0x5555555555555555ULL, 1);
            bb = delta_swap(bb, 0x3333333333333333ULL, 2);
            return delta_swap(bb, 0x0f0f0f0f0f0f0f0fULL, 4);
		}

		static uint64_t mirror_vertical(uint64_t bb)
		{
			bb = delta_swap(bb, 0x00ff00ff00ff00ffULL, 8);
            bb = delta_swap(bb, 0x0000ffff0000ffffULL, 16);
            return delta_swap(bb, 0x00000000ffffffffULL, 32);
		}

		static uint64_t mirror_diag_a1h8(uint64_t bb)
		{
			bb = delta_swap(bb, 0x00aa00aa00aa00aaULL, 7);
            bb = delta_swap(bb, 0x0000cccc0000ccccULL, 14);
            return delta_swap(bb, 0x00000000f0f0f0f0ULL, 28);
		}

		static uint64_t mirror_diag_a8h1(uint64_t bb)
		{
			bb = delta_swap(bb, 0x0055005500550055ULL, 9);
            bb = delta_swap(bb, 0x0000333300003333ULL, 18);
            return delta_swap(bb, 0x000000000f0f0f0fULL, 36);
		}

		static uint64_t rotate_clockwise(uint64_t bb) { return mirror_horizontal(mirror_diag_a1h8(bb)); }

    public:
        uint64_t player;
        uint64_t opponent;

		__Bitboard() : player(0ULL), opponent(0ULL) { }
        __Bitboard(uint64_t player, uint64_t opponent) : player(player), opponent(opponent) {  }
		uint64_t player_masked(uint64_t mask) const { return PEXT_64(player, mask); }
		uint64_t opponent_masked(uint64_t mask) const { return PEXT_64(opponent, mask); }
		uint64_t discs() const { return this->player | this->opponent; }
		uint64_t empties() const { return ~discs(); }
		int32_t player_disc_count() const { return std::popcount(this->player); }
		int32_t opponent_disc_count() const { return std::popcount(this->opponent); }
		int32_t disc_count() const { return std::popcount(discs()); }
		int32_t empty_count() const { return std::popcount(empties()); }
		uint64_t calc_player_mobility() const { return calc_mobility(this->player, this->opponent); }
		uint64_t calc_opponent_mobility() const { return calc_mobility(this->opponent, this->player); }
		uint64_t calc_flip_discs(const int8_t& coord) const { return reversi::calc_flip_discs(this->player, this->opponent, coord); }
		int32_t get_square_owner_at(int8_t coord) const { return 2 - 2 * ((this->player >> coord) & 1) - ((this->opponent >> coord) & 1);}

        void put_player_disc_at(int8_t coord)
		{
			uint64_t bit = COORD_TO_BIT[coord];
			this->player |= bit;

			if (this->opponent & bit)
				this->opponent ^= bit;
		}

		void put_opponent_disc_at(int8_t coord)
		{
			uint64_t bit = COORD_TO_BIT[coord];
			this->opponent |= bit;

			if (this->player & bit)
				this->player ^= bit;
		}

		void remove_disc_at(int8_t coord)
		{
			uint64_t bit = COORD_TO_BIT[coord];
			if (this->player & bit)
				this->player ^= bit;

			if (this->opponent & bit)
				this->opponent ^= bit;
		}

		void update(const int8_t& coord, const uint64_t& flip)
		{
			uint64_t player = this->player;
			this->player = this->opponent ^ flip;
			this->opponent = player | (COORD_TO_BIT[coord] | flip);
		}

		void undo(const int8_t& coord, const uint64_t& flip)
		{
			uint64_t player = this->player;
			this->player = this->opponent ^ (COORD_TO_BIT[coord] | flip);
			this->opponent = player | flip;
		}

		void swap()
		{
			uint64_t tmp = this->player;
			this->player = this->opponent;
			this->opponent = tmp;
		}

		void mirror_vertical()
		{
			this->player = __Bitboard::mirror_vertical(this->player);
			this->opponent = __Bitboard::mirror_vertical(this->opponent);
		}

		void mirror_horizontal()
		{
			this->player = __Bitboard::mirror_horizontal(this->player);
			this->opponent = __Bitboard::mirror_horizontal(this->opponent);
		}

		void mirror_diag_a1h8()
		{
			this->player = __Bitboard::mirror_diag_a1h8(this->player);
			this->opponent = __Bitboard::mirror_diag_a1h8(this->opponent);
		}

		void mirror_diag_a8h1()
		{
			this->player = __Bitboard::mirror_diag_a8h1(this->player);
			this->opponent = __Bitboard::mirror_diag_a8h1(this->opponent);
		}

		void rotate_clockwise()
		{
			this->player = __Bitboard::rotate_clockwise(this->player);
			this->opponent = __Bitboard::rotate_clockwise(this->opponent);
		}

		uint64_t calc_hash_code() const
		{
            uint64_t p = this->player, o = this->opponent;
            uint64_t crc = CRC32(0, p);
            return (crc << 32) | CRC32(crc, o);
		}
    };
}