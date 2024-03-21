#pragma once

#include "window_def.h"
#include "mycircle.h"

namespace QTreeNormal {

	static inline const uint64 MAX_D = 3;
	static inline const uint64 MAX_SPRIT = 2 * (MAX_D + 1);

	static constexpr uint64 sum_of_tree(const uint64 depth) {
		return ((1ULL << (2 * depth + 2)) - 1) / 3;
	}

	static inline constexpr uint64 CELL_NUM = sum_of_tree(MAX_D);

	static const uint64 offset[] = { 0, 1, 5, 21, 85 }; // (4^n - 1) / 3

	static inline const uint64 DX = WIN_W / MAX_SPRIT; // be careful! should be no remainder
	static inline const uint64 DY = WIN_H / MAX_SPRIT; // be careful! should be no remainder

	template <typename T>
	class QTree
	{
	public:
		void Push(std::weak_ptr<T> obj)
		{
			const auto p = obj.lock();
			if (!p) return;
			const auto c = p->GetCircle();
			const auto id = GetCellID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
			this->cell[id].push_front(std::move(obj));
			this->cell_size[id]++;
		}

		void Update() {
			// TODO
		}

	private:
		void HitTest(uint8_t L, uint8 z, std::deque<std::shared_ptr<T>>& deq)
		{
			const uint64_t id = GetCellID(L, z);
			// 衝突判定をしていく

			// スタックとセル
			for (auto itr = cell[id].begin(); itr != cell[id].end(); itr++) {
				const auto& p = itr->lock();
				if (!p) continue;
				const auto c1 = p->GetCircle();
				for (const auto& q : deq) {
					const auto c2 = q->GetCircle();
					if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
						p->hit_count++;
						q->hit_count++;
					}
				}
			}

			// セル同士
			for (auto itr1 = cell[id].begin(); itr1 != cell[id].end(); itr1++) {
				const auto& p = itr1->lock();
				if (!p) continue;
				const auto c1 = p->GetCircle();
				for (auto itr2 = std::next(itr1); itr2 != cell[id].end(); itr2++) {
					const auto& q = itr2->lock();
					if (!q) continue;
					const auto c2 = q->GetCircle();
					if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
						p->hit_count++;
						q->hit_count++;
					}
				}
			}

			// 深さが最大なら終了
			if (L == MAX_D)
				return;

			// スタックに積む
			for (auto& wp : cell[id]) {
				deq.push_back(std::move(wp.lock()));
			}
			// 子空間へ再帰
			for (uint8 i = 0; i < 4; i++) {
				QTree::HitTest(L + 1, (z << 2) + i, deq);
			}
			// スタックから降ろす
			const uint64_t size = cell_size[id];
			for (int i = 0; i < size; i++) {
				deq.pop_back();
			}
			return;
		}

	public:
		void HitTest() {
			std::deque<std::shared_ptr<T>> deq;
			const uint8 zero = 0;
			this->HitTest(zero, zero, deq);
		}

	private:
		std::forward_list<std::weak_ptr<T>> cell[CELL_NUM];
		uint64 cell_size[CELL_NUM] = { 0 };

		static uint8 X1(float x) {
			const uint8 ret = (uint8)(x / DX);
			return ret < 0 ? 0 : ret >= MAX_SPRIT ? MAX_SPRIT - 1 : ret;
		}

		static uint8 X2(float x) {
			const uint8 ret = -(uint8)(-x / DX); // ceil(x) = -floor(-x) if x is positive
			return ret < 0 ? 0 : ret >= MAX_SPRIT ? MAX_SPRIT - 1 : ret;
		}

		static uint8 Y1(float y) {
			const uint8 ret = (uint8)(y / DY);
			return ret < 0 ? 0 : ret >= MAX_SPRIT ? MAX_SPRIT - 1 : ret;
		}

		static uint8 Y2(float y) {
			const uint8 ret = -(uint8)(-y / DY); // ceil(x) = -floor(-x) if x is positive
			return ret < 0 ? 0 : ret >= MAX_SPRIT ? MAX_SPRIT - 1 : ret;
		}

		// 親のz値
		static constexpr uint8 GetParentZ(uint8 z)
		{
			return z >> 2;
		}


		// 深さとz値からセル番号を得る
		static constexpr uint64 GetCellID(uint8 L, uint8 idx)
		{
			return  offset[L] + idx;
		}

		// 4bitの入力を、bitを一つ飛ばしにして出力する。
		static constexpr uint8 BitSeparate4(uint8 n)
		{
			n = (n | (n << 2)) & (uint8)0x33;
			n = (n | (n << 1)) & (uint8)0x55;
			return n;
		}

		// 4bitの入力x, yからモートン番号を得る
		// x, yは最小分割単位のセルの上で左上から右にx, 下にy移動の意味
		static constexpr uint8 GetZ(uint8 x, uint8 y) {
			return (BitSeparate4(x) | (BitSeparate4(y) << 1));
		}

		// 対角の2点からセル番号を得る
		static uint64 GetCellID(float x1, float y1, float x2, float y2)
		{
			const auto ix1 = X1(x1);
			const auto iy1 = Y1(y1);
			const auto ix2 = X2(x2);
			const auto iy2 = Y2(y2);
			const uint8 x = ix1 xor ix2;
			const uint8 y = iy1 xor iy2;
			uint8 xy = x | y;
			// calc msb of xy
			xy = xy & (uint8)0xF0 ? xy & (uint8)0xF0 : xy;
			xy = xy & (uint8)0xCC ? xy & (uint8)0xCC : xy;
			xy = xy & (uint8)0xAA ? xy & (uint8)0xAA : xy;

			uint8 L = MAX_D;
			uint8 z = GetZ(ix1, iy1);
			if (xy == 0) {
				return offset[L] + z;
			}
			--L;
			z >>= 2;
			while ((xy & (uint8)1) == 0)
			{
				xy >>= 1;
				z >>= 2;
				--L;
			}
			return offset[L] + z;
		}
	};

} // namespace QTreeNormal
