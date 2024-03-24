#pragma once

#include "window_def.h"
#include "mycircle.h"

namespace QTree {

	// 最大深さdepthの四分木のノード数, 1 + 4 + 16 + ... + 4^depth = {(2^2)^(depth+1)) - 1} / 3
	static constexpr uint64_t sum_of_tree(const int64_t depth) {
		return ((((uint64_t)1) << (2 * depth + 2)) - 1) / 3;
	}

	// calc 2^depth, 最大深さdepthの四分木の一辺の分割数に相当
	static constexpr uint64_t MAX_SPLIT(const uint64_t depth) {
		return ((uint64_t)1) << depth;
	}

	static const uint64_t offset[16] = { 0, 1, 5, 21, 85, 341, 1365, 5461, 21845, 87381, 349525, 1398101, 5592405, 22369621, 89478485, 357913941 }; // (4^n - 1) / 3

	// for GetMSBPos(), magic-number table
	static const unsigned char s_abyLog2[32] = {
		 1,  2, 29,  3, 30, 20, 25,  4, 31, 23, 21, 11, 26, 13, 16,  5,
		32, 28, 19, 24, 22,  10, 12, 15, 27, 18,  9, 14, 17,  8,  7,  6
	};

	// 深さとz値からノード番号を得る
	static constexpr uint64 GetNodeID(uint64_t depth, uint64_t z_value)
	{
		return  offset[depth] + z_value;
	}

	// 最大16bitの入力を、bitを一つ飛ばしにして出力する。
	static constexpr uint64_t BitSeparate16(uint64_t n)
	{
		n = (n | (n << 8)) & (uint64_t)0x00FF00FF; // 9~16bit
		n = (n | (n << 4)) & (uint64_t)0x0F0F0F0F; // 5~8bit
		n = (n | (n << 2)) & (uint64_t)0x33333333; // 3~4 bit
		n = (n | (n << 1)) & (uint64_t)0x55555555; // 1~2bit
		return n;
	}

	// 最大16bitの入力x, yから32bitのz値を得る
	// x, yは最小分割単位のノードの上で左上から右にx, 下にy移動の意味
	static constexpr uint64_t GetZ(uint64_t x, uint64_t y)
	{
		return (BitSeparate16(x) | (BitSeparate16(y) << 1));
	}

	// 32bitの整数のMSBの位置を1-indexedで得る
	// 0x00000001の場合は1を返す
	// 0x00800F00の場合は24を返す
	// 0x00000000の場合は見定義！
	static constexpr uint64_t GetMSBPos(uint32_t uVal)
	{
		/* Propagates MSB to all the lower bits. */
		uVal |= (uVal >> 1); // 2~1 bit
		uVal |= (uVal >> 2); // 4~3 bit
		uVal |= (uVal >> 4); // 8~5 bit
		uVal |= (uVal >> 8); // 16~9 bit
		uVal |= (uVal >> 16); // 32~17 bit
		/* Turns off all the bits except MSB. */
		uVal >>= 1;
		uVal++;
		/* Parameter uVal must be a power of 2. */
		const uint32_t MAGIC = 0x07D6E531;
		uVal = (MAGIC * uVal) >> 27;

		return (int64_t)s_abyLog2[uVal & 0x1F];
	}

	namespace A2A {

		/*
		 * T型のオブジェクトを格納する、最大深さMAX_Dの領域四分木
		 * T型のオブジェクト同士の衝突判定を行う
		 * 最大 2^16 x 2^16 までの分割(=MAX_D <=15)に対応
		 */
		template <unsigned int MAX_D, typename T, MyCircle(T::* member_func)() const>
		class QTreeA2A
		{
		private:
			typedef std::function<void(T*, T*)> HitFunc;
			
		public:
			QTreeA2A(HitFunc hit_func)
				: hit_func(hit_func) {}

			/*
			 * オブジェクトを四分木に登録する
			 */
			void Push(T* obj)
			{
				const auto c = (obj->*member_func)();
				const auto id = GetNodeID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
				this->node[id].push_back(obj);
			}

			void Cleanup()
			{
				for (int i = 0; i < sum_of_tree(MAX_D); i++) {
					//node[i].resize(0);
					node[i].clear();
				}
			}

		private:
			/*
			 * 深さdepthでz値がz_valueを持つノードに登録されたオブジェクトに対して衝突判定を行う
			 */
			void HitTest(uint64_t depth, uint64_t z_value, std::vector<T*>& stack)
			{
				const uint64_t id = QTree::GetNodeID(depth, z_value);
				const std::vector<T*>& lst = node[id];
				const uint64_t lst_size = lst.size();

				// 衝突判定をしていく
				for (int i = 0; i < lst_size; i++) {
					T* const p1 = lst[i];
					const auto c1 = (p1->*member_func)();

					// スタックとノード
					for (int j = 0; j < stack.size(); j++) {
						const auto c2 = (stack[j]->*member_func)();
						if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
							hit_func(p1, stack[j]);
						}
					}
					// ノード同士
					for (int j = i + 1; j < lst_size; j++) {
						const auto c2 = (lst[j]->*member_func)();
						if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
							hit_func(p1, lst[j]);
						}
					}
				}

				// 深さが最大なら終了
				if (depth == MAX_D)
					return;

				// スタックに積む
				for (int i = 0; i < lst.size(); i++) {
					stack.push_back(lst[i]);
				}

				// 子空間へ再帰
				for (uint8 i = 0; i < 4; i++) {
					this->HitTest(depth + 1, (z_value << 2) + i, stack);
				}
				// スタックから降ろす
				for (int i = 0; i < lst_size; i++) {
					stack.pop_back();
				}
				return;
			}

		public:
			/*
			 * 登録されている全オブジェクトに対して衝突判定を行う
			 */
			void HitTest()
			{
				std::vector<T*> stack;
				this->HitTest(0, 0, stack);
			}

		private:
			std::vector<T*> node[sum_of_tree(MAX_D)]; // tree of vector<T*>
			HitFunc hit_func;

			// 対角の2点からノード番号を得る
			// (x1, y1) <= (x2, y2)とする
			static uint64 GetNodeID(float x1, float y1, float x2, float y2)
			{
				// 単位長方形の幅と高さ
				constexpr float DX = static_cast<float>(WIN_W) / MAX_SPLIT(MAX_D);
				constexpr float DY = static_cast<float>(WIN_H) / MAX_SPLIT(MAX_D);
				constexpr int64_t MAX_SPLIT_NUM = MAX_SPLIT(MAX_D);
				// 0 ~ MAX_SPLIT-1 のグリッド座標へ変換
				int64_t ix1 = (int64_t)(x1 / DX);
				int64_t iy1 = (int64_t)(y1 / DY);
				int64_t ix2 = (int64_t)(x2 / DX);
				int64_t iy2 = (int64_t)(y2 / DY);
				if (ix1 < 0 || ix2 >= MAX_SPLIT_NUM) {
					ix1 = ix1 < 0 ? 0 : ix1 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : ix1;
					ix2 = ix2 < 0 ? 0 : ix2 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : ix2;
				}
				if (iy1 < 0 || iy2 >= MAX_SPLIT_NUM) {
					iy1 = iy1 < 0 ? 0 : iy1 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : iy1;
					iy2 = iy2 < 0 ? 0 : iy2 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : iy2;
				}
				// 0 <= ix1, iy1, ix2, iy2 < MAX_SPLIT_NUM <= 2^16
				const uint64_t x = ix1 xor ix2;
				const uint64_t y = iy1 xor iy2;
				const uint64_t xy = x | y;
				const uint64_t z = GetZ(ix1, iy1);
				if (xy == 0) {
					return offset[MAX_D] + z;
				}
				uint64_t msb_pos = GetMSBPos(static_cast<uint32_t>(xy)); // 1-indexed, assume xy < 2^32
				uint64_t L = MAX_D - msb_pos;
				return offset[L] + (z >> (msb_pos << 1));
			}
		};
	} // namespace A2A

} // namespace QTreeA2A
