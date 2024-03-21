#pragma once

#include "window_def.h"
#include "mycircle.h"

namespace QTreeEx2 {

	static inline const uint8 MAX_D = 3;

	// 最大深さdepthの四分木のセル数, 1 + 4 + 16 + ... + 4^depth = {(2^2)^(depth+1)) - 1} / 3
	static constexpr uint64_t sum_of_tree(const uint64_t depth) {
		return ((((uint64_t)1) << (2 * depth + 2)) - 1) / 3;
	}

	// calc 2^depth, 最大深さdepthの四分木の一辺の分割数に相当
	static constexpr uint64_t MAX_SPLIT(const uint64_t depth) {
		return ((uint64_t)1) << depth;
	}

	static inline const uint64 z[MAX_SPLIT(MAX_D)][MAX_SPLIT(MAX_D)] = {
		{0, 1, 4, 5, 16, 17, 20, 21},
		{2, 3, 6, 7, 18, 19, 22, 23},
		{8, 9, 12, 13, 24, 25, 28, 29},
		{10, 11, 14, 15, 26, 27, 30, 31},
		{32, 33, 36, 37, 48, 49, 52, 53},
		{34, 35, 38, 39, 50, 51, 54, 55},
		{40, 41, 44, 45, 56, 57, 60, 61},
		{42, 43, 46, 47, 58, 59, 62, 63},
	};

	static inline const uint64 shift[8] = {
		0, 2, 4, 4,
		6, 6, 6, 6,
	};

	static inline const uint64 L[8] = {
		3, 2, 1, 1,
		0, 0, 0, 0,
	};

	// 0, 1, 5, 21
	static inline const uint64 offset[8] = {
		21, 5, 1, 1, 0, 0, 0, 0
	};

	static inline const uint64 normal_offset[4] = { 0, 1, 5, 21 };

	/*
	 * 分割数 8x8固定の四分木
	 * T*を扱い毎フレーム構築
	 * floorを高速化
	 * z値、MSBの計算に配列キャッシュを使用
	 * deque, listは自作のvectorを使用
	 */
	template <typename T>
	class QTreeEx2
	{
	private:
		class MyVector
		{
		private:
			T** _data;
			uint64_t _size;
			uint64_t _capacity;
		public:
			MyVector() : _data(nullptr), _size(0), _capacity(0) {}
			~MyVector() { delete[] _data; }
			MyVector(const MyVector& other) = delete;
			MyVector& operator=(const MyVector& other) = delete;
			MyVector(MyVector&& other)
				: _data(other._data), _size(other._size), _capacity(other._capacity)
			{
				other._data = nullptr;
				other._size = 0;
				other._capacity = 0;
			}
			MyVector& operator=(MyVector&& other)
			{
				if (this != &other) {
					delete[] _data;
					_data = other._data;
					_size = other._size;
					_capacity = other._capacity;
					other._data = nullptr;
					other._size = 0;
					other._capacity = 0;
				}
				return *this;
			}
			T* operator[](uint64_t i) const { return _data[i]; }
			void push_back(T* obj)
			{
				if (_size == _capacity) {
					_capacity = _capacity == 0 ? 1 : _capacity * 2;
					T** new_data = new T*[_capacity];
					for (uint64_t i = 0; i < _size; i++) {
						new_data[i] = _data[i];
					}
					delete[] _data;
					_data = new_data;
				}
				_data[_size++] = obj;
			}
			void resize(uint64_t new_size)
			{
				if (new_size < _size) {
					_size = new_size;
				}
				// never shrink
			}
			uint64_t size() const { return _size; }
		};
	public:
		void Push(T* obj)
		{
			const auto c = obj->GetCircle();
			const uint64 id = GetCellID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
			this->cell[id].push_back(obj);
		}

		void cleanup()
		{
			for (int i = 0; i < sum_of_tree(MAX_D); i++) {
				cell[i].resize(0);
			}
		}

	private:
		void HitTest(uint64_t depth, uint64_t z_value, MyVector& deq)
		{
			const uint64_t id = GetCellID(depth, z_value);
			const MyVector& lst = cell[id];

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
			for (uint64 i = 0; i < 4; i++) {
				this->HitTest(depth + 1, (z_value << 2) + i, deq);
			}
			// スタックから降ろす
			const uint64_t size = lst.size();
			deq.resize(deq.size() - size);
			return;
		}

	public:
		void HitTest() {
			MyVector deq;
			this->HitTest(0, 0, deq);
		}

	private:
		MyVector cell[sum_of_tree(MAX_D)];

		// 深さとz値からセル番号を得る
		static constexpr uint64 GetCellID(uint64 depth, uint64 idx)
		{
			return  normal_offset[depth] + idx;
		}

		// 円からセル番号を得る
		static uint64 GetCellID(float x1, float y1, float x2, float y2) {
			constexpr float DX = static_cast<float>(WIN_W) / MAX_SPLIT(MAX_D);
			constexpr float DY = static_cast<float>(WIN_H) / MAX_SPLIT(MAX_D);
			constexpr int64 MAX_SPLIT_NUM = MAX_SPLIT(MAX_D);

			auto ix1 = (int64)(x1 / DX);
			auto ix2 = (int64)(x2 / DX);
			auto iy1 = (int64)(y1 / DY);
			auto iy2 = (int64)(y2 / DY);
			if (ix1 < 0 || ix2 >= MAX_SPLIT_NUM) {
				ix1 = ix1 < 0 ? 0 : ix1 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : ix1;
				ix2 = ix2 < 0 ? 0 : ix2 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : ix2;
			}
			if (iy1 < 0 || iy2 >= MAX_SPLIT_NUM) {
				iy1 = iy1 < 0 ? 0 : iy1 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : iy1;
				iy2 = iy2 < 0 ? 0 : iy2 >= MAX_SPLIT_NUM ? MAX_SPLIT_NUM - 1 : iy2;
			}
			const uint64 ix = ix1 xor ix2;
			const uint64 iy = iy1 xor iy2;
			uint64 ixy = ix | iy;
			uint64 ret = z[ix1][iy1];
			return offset[ixy] + (ret >> shift[ixy]);
		}
	};

} // namespace QTreeEx2
