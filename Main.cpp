﻿# include <Siv3D.hpp> // Siv3D v0.6.14
#include "mycircle.h"
#include "window_def.h"
#include "naive.h"
// #include "qtree_normal.h"
// #include "qtree_extra.h"
#include "qtree_extra2.h"

static inline const int N_LEN = 5;
static inline constexpr int N_list[N_LEN] = {100, 1000, 5000, 10000, 15000};
static inline constexpr int N_MAX = N_list[N_LEN - 1];

void Main()
{
	Window::Resize(WIN_W, WIN_H);

	auto& rnd = GetDefaultRNG();
	rnd.seed(0U);

	// 最初から入れるN個の点を用意
	std::vector<MyObject> obj_for_naive;
	std::vector<std::shared_ptr<MyObject>> obj_for_qtree;
	// 乱数で初期化して格納しておく
	for (int i = 0; i < N_MAX; i++) {
		auto x = (float)(RandomUint32() % WIN_W);
		auto y = (float)(RandomUint32() % WIN_H);
		// auto r = (float)(RandomUint32() % 10 + 5);
		auto r = (float)(RandomUint32() % 3 + 5);
		obj_for_naive.push_back(MyObject(x, y, r));
		obj_for_qtree.push_back(std::move(std::make_shared<MyObject>(x, y, r)));
	}
	int now_idx = 0;
	const RectF button{ Arg::center(Scene::CenterF().x, Scene::CenterF().y), 50};

	uint64_t total_time = 0;
	uint64_t total_frame = 0;

	while (System::Update())
	{
		total_frame++;
		// clear hitcount
		for (int i = 0; i < N_MAX; i++) {
			obj_for_naive[i].hit_count = 0;
			obj_for_qtree[i]->hit_count = 0;
		}
		// change N
		if (button.leftClicked()) {
			now_idx = (now_idx + 1) % N_LEN;
			total_time = 0;
			total_frame = 1;
		}
		const int N = N_list[now_idx];

		auto t = Time::GetMicrosec();

		// hittest codes here!
		/*
		 * 1frameあたり16666microsec (10^6 / 60)
		 * 当たり判定には 1/5 の 3333 microsec程度しか使えない
		 * 描画には 1/10 の 1666 microsec 程度か使えない
		 */

		/*
		 * ナイーブな手法では、オブジェクトがvectorで管理されているとしておく
		 * 30000 microsec per frame with N = 10000
		 * 9100 microsec per frame with N = 5000
		 * 400 microsec per frame with N = 1000
		 * 5 microsec per frame with N = 100
		 */
		//NaiveTest(obj_for_naive);

		
		/*
		 * 14000 microsec per frame with N = 10000
		 * 3500 microsec per frame with N = 5000
		 * 250 microsec per frame with N = 1000
		 * 24 microsec per frame with N = 100
		 * 四分木の場合、オブジェクトはvector of weak_ptrで管理されているとする
		 */
		///*
		/*
		QTreeEx<MyObject> qtree;
		for (int i = 0; i < N; i++) {
			qtree.Push(obj_for_qtree[i].get());
		}
		qtree.UpdateReference();
		qtree.HitTest();
		//*/

		/*
		 * 15000 microsec per frame with N = 15000
		 * 6000 microsec per frame with N = 10000
		 * 1800 microsec per frame with N = 5000
		 * 150 microsec per frame with N = 1000
		 * 20 microsec per frame with N = 100
		 * 四分木を毎フレーム構築し直し、ポインタ配列で管理されているとする
		 */
		 ///*
		 ///*
		 QTreeEx2<MyObject> qtree;
		 for (int i = 0; i < N; i++) {
			 qtree.Push(obj_for_qtree[i].get());
		 }
		 qtree.HitTest();
		 //*/
		
		total_time += Time::GetMicrosec() - t;

		// draw
		auto t2 = Time::GetMicrosec();
		Scene::SetBackground(Palette::Gray);
		ClearPrint();
		Print << U"N: " << N << U" objects";
		Print << U"average microsec per frame: " << total_time / total_frame;
		for (int i = 0; i < N; i++) {
			const auto& c = obj_for_naive[i].GetCircle();
			bool hit = (obj_for_naive[i].hit_count > 0) || (obj_for_qtree[i]->hit_count > 0);
			Circle{ c.x, c.y, c.r }.draw(hit ? Palette::Lightpink : Palette::Lightgreen);
		}
		Print << U"Time to draw: " << Time::GetMicrosec() - t2 << U" microsec";
		button.draw(Palette::White);
	}
}
