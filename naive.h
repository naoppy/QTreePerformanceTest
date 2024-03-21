#pragma once

#include "mycircle.h"

void NaiveTest(std::vector<MyObject>& obj, int32_t N) {
	// hittest
	for (int i = 0; i < N; i++) {
		const auto& c1 = obj[i].GetCircle();
		for (int j = i + 1; j < N; j++) {
			const auto& c2 = obj[j].GetCircle();
			if (HitTestCircle(c1.x, c1.y, c1.r, c2.x, c2.y, c2.r)) {
				obj[i].hit_count++;
				obj[j].hit_count++;
			}
		}
	}
}
