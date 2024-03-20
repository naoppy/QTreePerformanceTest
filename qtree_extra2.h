#pragma once

#include "window_def.h"
#include "mycircle.h"

static inline const uint64 MAX_D = 3;
static inline const uint64 MAX_SPRIT = 2 * (MAX_D + 1);

static constexpr uint64 sum_of_tree(const uint64 depth) {
	return ((1ULL << (2 * depth + 2)) - 1) / 3;
}

static inline constexpr uint64 CELL_NUM = sum_of_tree(MAX_D);

static inline const float DX = (float)WIN_W / MAX_SPRIT;
static inline const float DY = (float)WIN_H / MAX_SPRIT;

static inline const uint8 z[CELL_NUM][CELL_NUM] = {
	{0, 1, 4, 5, 16, 17, 20, 21},
	{2, 3, 6, 7, 18, 19, 22, 23},
	{8, 9, 12, 13, 24, 25, 28, 29},
	{10, 11, 14, 15, 26, 27, 30, 31},
	{32, 33, 36, 37, 48, 49, 52, 53},
	{34, 35, 38, 39, 50, 51, 54, 55},
	{40, 41, 44, 45, 56, 57, 60, 61},
	{42, 43, 46, 47, 58, 59, 62, 63},
};

static inline const uint8 shift[8] = {
	0, 2, 4, 4,
	6, 6, 6, 6,
};

static inline const uint8 L[8] = {
	3, 2, 1, 1,
	0, 0, 0, 0,
};

// 0, 1, 5, 21
static inline const uint8 offset[8] = {
	21, 5, 1, 1, 0, 0, 0, 0
};

static inline const uint64 normal_offset[4] = { 0, 1, 5, 21 };

/*
 * 変更点: std::listにした
 * expiredの確認を大幅に省いた
 * floorを高速化
 * 配列キャッシュを使用
 * deque, listをiteratorにしてT*型に変更
 */
template <typename T>
class QTreeEx2
{
public:
	void Push(T* obj)
	{
		const auto c = obj->GetCircle();
		const auto id = GetCellID(c.x, c.y, c.r);
		this->cell[id].push_back(obj);
	}

	void Update() {
		// TODO
	}

private:
	void HitTest(uint8_t depth, uint8 z_value, std::vector<T*>& deq)
	{
		const uint64_t id = GetCellID(depth, z_value);
		const std::vector<T*>& lst = cell[id];

		// 衝突判定をしていく
		for (int i = 0; i < lst.size(); i++) {
			T* const p1 = lst[i];
			const auto c1 = p1->GetCircle();

			// スタックとセル
			for (int j = 0; j < deq.size(); j++) {
				const auto c2 = deq[j]->GetCircle();
				if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
					p1->hit_count++;
					deq[j]->hit_count++;
				}
			}
			// セル同士
			for (int j = i + 1; j < lst.size(); j++) {
				const auto c2 = lst[j]->GetCircle();
				if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
					p1->hit_count++;
					lst[j]->hit_count++;
				}
			}
		}

		// 深さが最大なら終了
		if (depth == MAX_D)
			return;

		// スタックに積む
		for (int i = 0; i < lst.size(); i++) {
			deq.push_back(lst[i]);
		}

		// 子空間へ再帰
		for (uint8 i = 0; i < 4; i++) {
			this->HitTest(depth + 1, (z_value << 2) + i, deq);
		}
		// スタックから降ろす
		// TODO: resize()のほうがいいかもしれない
		const uint64_t size = lst.size();
		for (int i = 0; i < size; i++) {
			deq.pop_back();
		}
		return;
	}

public:
	// 衝突判定は前に、UpdateReferenceを呼び出しておくこと
	void HitTest() {
		std::vector<T*> deq;
		const uint8 zero = 0;
		this->HitTest(zero, zero, deq);
	}

private:
	std::vector<T*> cell[CELL_NUM];

	// 深さとz値からセル番号を得る
	static constexpr uint64 GetCellID(uint64 depth, uint64 idx)
	{
		return  normal_offset[depth] + idx;
	}

	static uint64 GetCellID(float x, float y, float r) {
		auto ix1 = (uint8)((x - r) / DX);
		auto ix2 = (uint8)((x + r) / DX);
		if (ix1 < 0 || ix2 >= MAX_SPRIT) {
			ix1 = ix1 < 0 ? 0 : ix1 >= MAX_SPRIT ? MAX_SPRIT - 1 : ix1;
			ix2 = ix2 < 0 ? 0 : ix2 >= MAX_SPRIT ? MAX_SPRIT - 1 : ix2;
		}
		auto iy1 = (uint8)((y - r) / DY);
		auto iy2 = (uint8)((y + r) / DY);
		if (iy1 < 0 || iy2 >= MAX_SPRIT) {
			iy1 = iy1 < 0 ? 0 : iy1 >= MAX_SPRIT ? MAX_SPRIT - 1 : iy1;
			iy2 = iy2 < 0 ? 0 : iy2 >= MAX_SPRIT ? MAX_SPRIT - 1 : iy2;
		}
		return GetCellID(ix1, iy1, ix2, iy2);
	}

	// 対角の2点からセル番号を得る
	static uint64 GetCellID(uint8 ix1, uint8 iy1, uint8 ix2, uint8 iy2)
	{
		const uint64 x = ix1 xor ix2;
		const uint64 y = iy1 xor iy2;
		uint64 xy = x | y;
		uint64 ret = z[ix1][iy1];
		return offset[xy] + (ret >> shift[xy]);
	}
};
