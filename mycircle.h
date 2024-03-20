#pragma once

// 当たり判定のプリミティブ
struct MyCircle
{
	float x, y, r;
};

// 当たり判定を持つオブジェクト
class MyObject
{
private:
	float x, y, r;
public:
	// 当たり判定を返す
	MyCircle GetCircle() const
	{
		return MyCircle{ x, y, r };
	}
	int hit_count = 0;
	MyObject(float x, float y, float r) : x{ x }, y{ y }, r{ r } {}
};

inline bool HitTestCircle(const float x1, const float y1, const float r1, const float x2, const float y2, const float r2)
{
	const float dx = x2 - x1, dy = y2 - y1;
	const float r = r1 + r2;
	return dx * dx + dy * dy <= r * r;
}
