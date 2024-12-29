// TEST-SARRAY.CPP
// Copyright (c) A.Sobolev 2023, 2024
// Тестирование контейнеров (SVector, SArray, etc)
//
#include <pp.h>
#pragma hdrstop

struct TestFixtureSArray {
	TestFixtureSArray()
	{
		memzero(Tail, sizeof(Tail));
		for(uint i = 0; i < SIZEOFARRAY(Sa); ++i) {
			Sa[i] = SLS.GetTLA().Rg.GetUniformInt(119000);
		}
	}
	uint GetSaCount() const { return SIZEOFARRAY(Sa); }
	long   Sa[7000];
	long   Tail[17]; // хвост, который не должен измениться ни при каких условиях
};

SLTEST_FIXTURE(SArray, TestFixtureSArray)
{
	uint   i;
	SArray * p_list = new SArray(sizeof(long));
	const uint cs1 = 0;
	const uint cc1 = F.GetSaCount() / 4;
	const uint cs2 = cc1;
	const uint cc2 = cc1 + F.GetSaCount() / 2;
	const uint cs3 = cc2;
	const uint cc3 = cc2 + F.GetSaCount() / 4;
	//
	// Четверть элементов вставляем в конец списка (самая простая и быстрая операция)
	//
	for(i = cs3; i < cc3; i++) {
		SLCHECK_NZ(p_list->insert(&F.Sa[i]));
	}
	//
	// Четверть элементов вставляем в начало списка (при этом предшествующая четверть будет постепенно сдвигаться)
	//
	for(i = cs1; i < cc1; i++) {
		SLCHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Оставшуюся половину вставляем в середину списка (опять заставляет массив сдвигать хвостовые члены)
	//
	for(i = cs2; i < cc2; i++) {
		SLCHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Тестируем функцию incPointerSafe
	//
	{
		p_list->setPointer(0);
		for(int _inc = -3; _inc < (int)p_list->getCount() + 7; _inc++) {
			const uint prev_ptr = p_list->getPointer();
			SLCHECK_EQ(p_list->incPointerSafe(_inc), prev_ptr);
			SLCHECK_CRANGE(p_list->getPointer(), 0UL, p_list->getCount()-1);
		}
		SLCHECK_EQ(p_list->getPointer(), p_list->getCount()-1);
	}
	//
	// Теперь сравниваем элементы list с элементами F.Sa. Они должны быть в том же порядки и равны
	//
	if(SLCHECK_EQ((ulong)p_list->getCount(), (ulong)F.GetSaCount())) {
		for(i = 0; i < F.GetSaCount(); i++) {
			SLCHECK_EQ(*(long *)p_list->at(i), F.Sa[i]);
		}
		long p;
		//
		// Далее, занимаемся поиском. Перебираем все значения от 0 до F.GetSaCount().
		// Сколько раз они встречаются в F.Sa столько раз мы должны их найти в p_list
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong ls_ss = 0;
			ulong ls_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++ls_ss;
			for(i = 0; p_list->lsearch(&p, &i, CMPF_LONG); i++)
				++ls_sl;
			SLCHECK_EQ(ls_ss, ls_sl);
		}
		//
		// Реверс списка
		//
		p_list->reverse(0, p_list->getCount());
		for(i = 0; i < F.GetSaCount(); i++) {
			SLCHECK_EQ(*static_cast<const long *>(p_list->at(i)), F.Sa[F.GetSaCount()-i-1]);
		}
		//
		// Сортируем список p_list и тестируем правильность сортировки
		//
		p_list->sort(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLCHECK_LE(*static_cast<const long *>(p_list->at(i)), *static_cast<const long *>(p_list->at(i+1)));
		}
		//
		// Перемешиваем элементы массива случайным образом
		//
		p_list->shuffle();
		{
			//
			// Убеждаемся, что массив более не отсортирован
			//
			int    sorted = 1;
			for(i = 0; sorted && i < p_list->getCount()-1; i++) {
				if(*static_cast<const long *>(p_list->at(i)) > *static_cast<const long *>(p_list->at(i+1)))
					sorted = 0;
			}
			SLCHECK_EQ(sorted, 0);
		}
		//
		// Теперь сортируем список p_list методом sort2 и тестируем правильность сортировки
		//
		p_list->sort2(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLCHECK_LE(*static_cast<const long *>(p_list->at(i)), *static_cast<const long *>(p_list->at(i+1)));
		}
		//
		// Тестируем бинарный поиск (алгоритм аналогичен тесту последовательного поиска)
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong bs_ss = 0;
			ulong bs_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++bs_ss;
			if(p_list->bsearch(&p, &i, CMPF_LONG)) {
				++bs_sl;
				//
				// Бинарный поиск может найти только самый первый элемент. Остальные элементы
				// выискиваем последовательным перебором
				//
				while(i < p_list->getCount() && *(long *)p_list->at(++i) == p)
					++bs_sl;
			}
			SLCHECK_EQ(bs_ss, bs_sl);
		}
	}
	// @todo Тест удаления элементов из массива
	delete p_list;
	return CurrentStatus;
}
//
// Descr: Тест класса SVector.
//   На момент создания полностью повторяет тест SArray. Необходимо убедиться, что
//   SVector в большинстве аспектов функционирует также правильно как и SArray.
//
SLTEST_FIXTURE(SVector, TestFixtureSArray)
{
	uint   i;
	SVector * p_list = new SVector(sizeof(long));
	const uint cs1 = 0;
	const uint cc1 = F.GetSaCount() / 4;
	const uint cs2 = cc1;
	const uint cc2 = cc1 + F.GetSaCount() / 2;
	const uint cs3 = cc2;
	const uint cc3 = cc2 + F.GetSaCount() / 4;
	//
	// Четверть элементов вставляем в конец списка (самая простая и быстрая операция)
	//
	for(i = cs3; i < cc3; i++) {
		SLCHECK_NZ(p_list->insert(&F.Sa[i]));
	}
	//
	// Четверть элементов вставляем в начало списка (при этом предшествующая четверть будет постепенно сдвигаться)
	//
	for(i = cs1; i < cc1; i++) {
		SLCHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Оставшуюся половину вставляем в середину списка (опять заставляет массив сдвигать хвостовые члены)
	//
	for(i = cs2; i < cc2; i++) {
		SLCHECK_NZ(p_list->atInsert(i, &F.Sa[i]));
	}
	//
	// Тестируем функцию incPointerSafe
	//
	{
		p_list->setPointer(0);
		for(int _inc = -3; _inc < (int)p_list->getCount() + 7; _inc++) {
			const uint prev_ptr = p_list->getPointer();
			SLCHECK_EQ(p_list->incPointerSafe(_inc), prev_ptr);
			SLCHECK_CRANGE(p_list->getPointer(), 0UL, p_list->getCount()-1);
		}
		SLCHECK_EQ(p_list->getPointer(), p_list->getCount()-1);
	}
	//
	// Теперь сравниваем элементы list с элементами F.Sa. Они должны быть в том же порядки и равны
	//
	if(SLCHECK_EQ((ulong)p_list->getCount(), (ulong)F.GetSaCount())) {
		for(i = 0; i < F.GetSaCount(); i++) {
			SLCHECK_EQ(*(long *)p_list->at(i), F.Sa[i]);
		}
		long p;
		//
		// Далее, занимаемся поиском. Перебираем все значения от 0 до F.GetSaCount().
		// Сколько раз они встречаются в F.Sa столько раз мы должны их найти в p_list
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong ls_ss = 0;
			ulong ls_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++ls_ss;
			for(i = 0; p_list->lsearch(&p, &i, CMPF_LONG); i++)
				++ls_sl;
			SLCHECK_EQ(ls_ss, ls_sl);
		}
		//
		// Реверс списка
		//
		p_list->reverse(0, p_list->getCount());
		for(i = 0; i < F.GetSaCount(); i++) {
			SLCHECK_EQ(*(long *)p_list->at(i), F.Sa[F.GetSaCount()-i-1]);
		}
		//
		// Сортируем список p_list и тестируем правильность сортировки
		//
		p_list->sort(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLCHECK_LE(*(long *)p_list->at(i), *(long *)p_list->at(i+1));
		}
		//
		// Перемешиваем элементы массива случайным образом
		//
		p_list->shuffle();
		{
			//
			// Убеждаемся, что массив более не отсортирован
			//
			bool sorted = true;
			for(i = 0; sorted && i < p_list->getCount()-1; i++) {
				if(*(long *)p_list->at(i) > *(long *)p_list->at(i+1))
					sorted = false;
			}
			SLCHECK_Z(sorted);
		}
		//
		// Теперь сортируем список p_list методом sort2 и тестируем правильность сортировки
		//
		p_list->sort2(CMPF_LONG);
		for(i = 0; i < p_list->getCount()-1; i++) {
			SLCHECK_LE(*(long *)p_list->at(i), *(long *)p_list->at(i+1));
		}
		//
		// Тестируем бинарный поиск (алгоритм аналогичен тесту последовательного поиска)
		//
		for(p = 0; p < (long)F.GetSaCount(); p++) {
			ulong bs_ss = 0;
			ulong bs_sl = 0;
			for(i = 0; i < F.GetSaCount(); i++)
				if(F.Sa[i] == p)
					++bs_ss;
			if(p_list->bsearch(&p, &i, CMPF_LONG)) {
				++bs_sl;
				//
				// Бинарный поиск может найти только самый первый элемент. Остальные элементы
				// выискиваем последовательным перебором
				//
				while(i < p_list->getCount() && *(long *)p_list->at(++i) == p)
					++bs_sl;
			}
			SLCHECK_EQ(bs_ss, bs_sl);
		}
	}
	// @todo Тест удаления элементов из массива
	ZDELETE(p_list);
	// @v11.7.0 {
	{
		const long raw_items[] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 11, 11 }; // Между 9 и 11 - пропуск!
		TSVector <long> vec;
		for(uint i = 0; i < SIZEOFARRAY(raw_items); i++) {
			vec.insert(&raw_items[i]);
		}
		long key;
		uint pos = 0;
		{
			key = 1;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 0U);
		}
		{
			key = 2;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 2U);
		}
		{
			key = 3;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 4U);
		}
		{
			key = 4;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 6U);
		}
		{
			key = 5;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 8U);
		}
		{
			key = 6;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 10U);
		}
		{
			key = 7;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 12U);
		}
		{
			key = 8;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 14U);
		}
		{
			key = 9;
			SLCHECK_NZ(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 16U);
		}
		{
			key = 19; // нет такого элемента
			SLCHECK_Z(vec.bsearch(&key, &pos, PTR_CMPFUNC(long)));
		}
		//
		{
			key = 1;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 0U);
		}
		{
			key = 2;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 2U);
		}
		{
			key = 3;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 4U);
		}
		{
			key = 4;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 6U);
		}
		{
			key = 5;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 8U);
		}
		{
			key = 6;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 10U);
		}
		{
			key = 7;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 12U);
		}
		{
			key = 8;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 14U);
		}
		{
			key = 9;
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 16U);
		}
		{
			key = 10; // Такого элемента нет в списке - bsearchGe должна найти следующий (11)
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 18U);
			SLCHECK_LE(key, vec.at(pos));
		}
		{
			key = -1; // Такого элемента нет в списке - bsearchGe должна найти следующий (1)
			SLCHECK_NZ(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
			SLCHECK_EQ(pos, 0U);
			SLCHECK_LE(key, vec.at(pos));
		}
		{
			key = 19; // нет такого элемента
			SLCHECK_Z(vec.bsearchGe(&key, &pos, PTR_CMPFUNC(long)));
		}
	}
	// } @v11.7.0 
	{
		// @v12.1.3 Тест перемещения элементов внутри вектора
		const uint _count = 100;
		class VectorBuilder {
		public:
			static void Build(TSVector <int> & rV)
			{
				for(int i = 1; i <= _count; i++) {
					rV.insert(&i);
				}
			}
		};
		{
			TSVector <int> v;
			VectorBuilder::Build(v);
			for(uint i = 0; i < v.getCount()-1; i++) {
				uint new_pos = 0;
				SLCHECK_NZ(v.moveItem(i, 0, &new_pos));
				SLCHECK_EQ(new_pos, i+1);
				for(uint j = 0; j < v.getCount(); j++) {
					if(j == new_pos) {
						assert(v.at(j) == 1);
						SLCHECK_EQ(v.at(j), 1);
					}
					else if(j < new_pos) {
						if(j > 0) {
							assert(v.at(j) == v.at(j-1)+1);
							SLCHECK_EQ(v.at(j), v.at(j-1)+1);
						}
					}
					else if(j > new_pos+1) {
						assert(v.at(j) == v.at(j-1)+1);
						SLCHECK_EQ(v.at(j), v.at(j-1)+1);
					}
				}
			}
		}
		{
			TSVector <int> v;
			VectorBuilder::Build(v);
			uint i = v.getCount();
			if(i) do {
				i--;
				uint new_pos = 0;
				SLCHECK_NZ(v.moveItem(i, 1, &new_pos));
				SLCHECK_EQ(new_pos, i-1);
				for(uint j = 0; j < v.getCount(); j++) {
					if(j == new_pos) {
						assert(v.at(j) == _count);
						SLCHECK_EQ(v.at(j), static_cast<int>(_count));
					}
					else if(j > new_pos+1) {
						assert(v.at(j) == v.at(j-1)+1);
						SLCHECK_EQ(v.at(j), v.at(j-1)+1);
					}
					else if(j > 0 && j < new_pos) {
						assert(v.at(j) == v.at(j-1)+1);
						SLCHECK_EQ(v.at(j), v.at(j-1)+1);
					}
				}
			} while(i > 1);
		}
		{
			TSVector <int> v;
			VectorBuilder::Build(v);
			SLCHECK_EQ(v.at(0), 1);
			v.moveItemTo(0, _count-1);
			SLCHECK_EQ(v.at(_count-1), 1);
			v.moveItemTo(_count-1, 1);
			SLCHECK_EQ(v.at(1), 1);
			v.moveItemTo(_count-1, 0);
			SLCHECK_EQ(v.at(0), 100);
			{
				// Этот блок зависит от верхних операторов - будьте внимательны, если поменяете действия выше!
				SLCHECK_EQ(v.at(50-1), 49);
				v.moveItemTo(50-1, 51);
				SLCHECK_EQ(v.at(51), 49);
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(TSHashCollection)
{
	struct Sample_HashTableEntry {
		const void * GetHashKey(const void * pCtx, uint * pKeySize) const
		{
			ASSIGN_PTR(pKeySize, CityEn.Len());
			return CityEn;
		}
		SString CityEn;
		SString CityRu;
	};
	TSCollection <Sample_HashTableEntry> city_list;
	TSHashCollection <Sample_HashTableEntry> city_hash_table(1024, 0);

	SString in_file_path;
	SString left, right;
	SString line_buf;
	SLS.QueryPath("testroot", in_file_path);
	in_file_path.SetLastSlash().Cat("data").SetLastSlash().Cat("city-enru-pair.csv");
	SFile f_in(in_file_path, SFile::mRead);
	THROW(SLCHECK_NZ(f_in.IsValid()));
	{
		while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
			if(line_buf.Divide(';', left, right) > 0 && left.NotEmptyS()) {
				Sample_HashTableEntry * p_new_entry = city_list.CreateNewItem();
				THROW(SLCHECK_NZ(p_new_entry));
				p_new_entry->CityEn = left;
				p_new_entry->CityRu = right.Strip();
			}
		}
	}
	{
		for(uint i = 0; i < city_list.getCount(); i++) {
			Sample_HashTableEntry * p_hash_entry = new Sample_HashTableEntry(*city_list.at(i));
			int hash_table_put_result = city_hash_table.Put(p_hash_entry, false);
			THROW(SLCHECK_EQ(hash_table_put_result, 1));
		}
	}
	{
		for(uint i = 0; i < city_list.getCount(); i++) {
			const Sample_HashTableEntry * p_entry = city_list.at(i);
			const Sample_HashTableEntry * p_hash_entry = city_hash_table.Get(p_entry->CityEn, p_entry->CityEn.Len());
			THROW(SLCHECK_NZ(p_hash_entry));
			THROW(SLCHECK_EQ(p_hash_entry->CityEn, p_entry->CityEn));
			THROW(SLCHECK_EQ(p_hash_entry->CityRu, p_entry->CityRu));
		}
	}
	{
		uint   _count = 0;
		Sample_HashTableEntry * p_hash_entry = 0;
		uint   prev_idx = 0;
		for(uint idx = 0; city_hash_table.Enum(&idx, &p_hash_entry);) {
			assert(idx > prev_idx);
			prev_idx = idx;
			SLCHECK_NZ(p_hash_entry);
			if(p_hash_entry) {
				_count++;	
				const Sample_HashTableEntry * p_found_entry = 0;
				for(uint i = 0; !p_found_entry && i < city_list.getCount(); i++) {
					const Sample_HashTableEntry * p_entry = city_list.at(i);
					if(p_entry->CityEn == p_hash_entry->CityEn) {
						p_found_entry = p_entry;
					}
				}
				SLCHECK_NZ(p_found_entry);
				if(p_found_entry) {
					THROW(SLCHECK_EQ(p_hash_entry->CityEn, p_found_entry->CityEn));
					THROW(SLCHECK_EQ(p_hash_entry->CityRu, p_found_entry->CityRu));
				}
			}
		}
		SLCHECK_EQ(_count, city_list.getCount());
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
