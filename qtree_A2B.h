#pragma once

#include "qtree_A2A.h"

namespace QTree::A2B {
	/*
	 * 最大深さMAX_Dの領域四分木
	 * A型とB型との衝突判定を行う
	 * 最大 2^16 x 2^16 までの分割(=MAX_D <=15)に対応
	 */
	template <unsigned int MAX_D, typename A, MyCircle(A::* a_member_func)() const, typename B, MyCircle(B::* b_member_func)() const>
	class QTreeA2B
	{
	private:
		typedef std::function<void(A*, B*)> HitFunc;
	public:
		QTreeA2B(HitFunc hit_func)
			: hit_func(hit_func) {}

		/*
		 * A型オブジェクトを四分木に登録する
		 */
		void PushA(A* obj)
		{
			const auto c = (obj->*a_member_func)();
			const auto id = GetNodeID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
			this->a_node[id].push_back(obj);
		}

		/*
		 * B型オブジェクトを四分木に登録する
		 */
		void PushB(B* obj)
		{
			const auto c = (obj->*b_member_func)();
			const auto id = GetNodeID(c.x - c.r, c.y - c.r, c.x + c.r, c.y + c.r);
			this->b_node[id].push_back(obj);
		}

		void Cleanup()
		{
			for (int i = 0; i < sum_of_tree(MAX_D); i++) {
				a_node[i].clear();
				b_node[i].claer();
			}
		}

	private:
		/*
		 * 深さdepthでz値がz_valueを持つノードに登録されたオブジェクトに対して衝突判定を行う
		 */
		void HitTest(uint64_t depth, uint64_t z_value, std::vector<A*>& a_stack, std::vector<B*>& b_stack)
		{
			const uint64_t id = QTree::GetNodeID(depth, z_value);
			const std::vector<A*>& a_lst = a_node[id];
			const std::vector<B*>& b_lst = b_node[id];

			// BスタックとAノード
			for (int i = 0; i < a_lst.size(); i++) {
				A* const p1 = a_lst[i];
				const auto c1 = (p1->*a_member_func)();
				for (int j = 0; j < b_stack.size(); j++) {
					const auto c2 = (b_stack[j]->*b_member_func)();
					if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
						hit_func(p1, b_stack[j]);
					}
				}
			}

			// AスタックとBノード
			for (int i = 0; i < b_lst.size(); i++) {
				B* const p1 = b_lst[i];
				const auto c1 = (p1->*b_member_func)();
				for (int j = 0; j < a_stack.size(); j++) {
					const auto c2 = (a_stack[j]->*a_member_func)();
					if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
						hit_func(a_stack[j], p1);
					}
				}
			}

			// AノードとBノード
			for (int i = 0; i < a_lst.size(); i++) {
				A* const p1 = a_lst[i];
				const auto c1 = (p1->*a_member_func)();
				for (int j = 0; j < b_lst.size(); j++) {
					const auto c2 = (b_lst[j]->*b_member_func)();
					if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
						hit_func(p1, b_lst[j]);
					}
				}
			}

			// 深さが最大なら終了
			if (depth == MAX_D)
				return;

			// スタックに積む
			for (int i = 0; i < a_lst.size(); i++) {
				a_stack.push_back(a_lst[i]);
			}
			for (int i = 0; i < b_lst.size(); i++) {
				b_stack.push_back(b_lst[i]);
			}

			// 子空間へ再帰
			for (uint8 i = 0; i < 4; i++) {
				this->HitTest(depth + 1, (z_value << 2) + i, a_stack, b_stack);
			}

			// スタックから降ろす
			for (int i = 0; i < a_lst.size(); i++) {
				a_stack.pop_back();
			}
			for (int i = 0; i < b_lst.size(); i++) {
				b_stack.pop_back();
			}
			return;
		}

	public:
		/*
		 * 登録されている全オブジェクトに対して衝突判定を行う
		 */
		void HitTest()
		{
			std::vector<A*> a_stack;
			std::vector<B*> b_stack;
			this->HitTest(0, 0, a_stack, b_stack);
		}

	private:
		std::vector<A*> a_node[sum_of_tree(MAX_D)];
		std::vector<B*> b_node[sum_of_tree(MAX_D)];
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
} // namespace QTree::A2B
