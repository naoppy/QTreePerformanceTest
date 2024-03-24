#pragma once

#include "qtree_A2A.h"

namespace QTree::A {
	/*
	 * 最大深さMAX_Dの領域四分木
	 * A型と小数のB型との衝突判定を行う
	 * 最大 2^16 x 2^16 までの分割(=MAX_D <=15)に対応
	 */
	template <unsigned int MAX_D, typename A, MyCircle(A::* a_member_func)() const>
	class QTreeA
	{
	private:
		typedef std::function<void(A*)> HitFunc;
	public:
		/*
		 * A型オブジェクトを四分木に登録する
		 */
		void PushA(A* obj)
		{
			const auto c = (obj->*a_member_func)();
			const auto id = GetNodeID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
			this->a_node[id].push_back(obj);
		}

		void Cleanup()
		{
			for (int i = 0; i < sum_of_tree(MAX_D); i++) {
				a_node[i].clear();
			}
		}

	private:
		void HitTestRecUp(uint64_t id, MyCircle& hitbox, HitFunc& hit_func) {
			const std::vector<A*>& a_lst = a_node[id];
			for (int i = 0; i < a_lst.size(); i++) {
				A* const p1 = a_lst[i];
				const auto c1 = (p1->*a_member_func)();
				if (HitTestCircle(c1.x, c1.y, c1.r, hitbox.x, hitbox.y, hitbox.r)) {
					hit_func(p1);
				}
			}
			if (id == 0) {
				return;
			}
			HitTestRecUp((id - 1) >> 2, hitbox, hit_func);
		}

		void HitTestRecDown(uint64_t id, MyCircle& hitbox, HitFunc& hit_func) {
			const std::vector<A*>& a_lst = a_node[id];
			for (int i = 0; i < a_lst.size(); i++) {
				A* const p1 = a_lst[i];
				const auto c1 = (p1->*a_member_func)();
				if (HitTestCircle(c1.x, c1.y, c1.r, hitbox.x, hitbox.y, hitbox.r)) {
					hit_func(p1);
				}
			}
			if (id >= sum_of_tree(MAX_D - 1)) {
				return;
			}
			for (int i = 1; i <= 4; i++) {
				HitTestRecDown((id << 2) + i, hitbox, hit_func);
			}
		}

	public:
		/*
		 * 登録されている全オブジェクトに対して衝突判定を行う
		 */
		void HitTest(MyCircle hitbox, HitFunc hit_func)
		{
			auto id = GetNodeID(hitbox.x - hitbox.r, hitbox.y - hitbox.r, hitbox.x + hitbox.r, hitbox.y + hitbox.r);
			if (id != 0) {
				HitTestRecUp((id - 1) >> 2, hitbox, hit_func);
			}
			HitTestRecDown(id, hitbox, hit_func);
		}

	private:
		std::vector<A*> a_node[sum_of_tree(MAX_D)];

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
} // namespace QTree::A2B
