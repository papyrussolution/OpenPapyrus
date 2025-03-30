// PPUTIL64.CPP
// Copyright (c) A.Sobolev 2024, 2025
// @codepage UTF-8
// @construction
// Инициирующая цель модуля: сделать 64-битную утилиту для формирования словарей sentencepiece по набору наименований товаров.
//   Так как набор большой, то в 32-битном варианте не хватает памяти.
//
#include <pp.h>
#include <..\OSF\sentencepiece\src\sentencepiece_processor.h>
#include <..\OSF\sentencepiece\src\sentencepiece_trainer.h>

int BuildSentencePieceDictionary(const char * pPath, const char * pBasisName, int vocabSize)
{
	int    ok = 1;
	bool   debug_mark = false;
	SString temp_buf;
	//DbProvider * p_dict = CurDict;
	//if(p_dict) 
	{
		SString basis_path_wo_ext; // Базовое имя файла с путем, но без расширения (для формирования прочих имен файлов)
		SString raw_input_file_path;
		SString model_file_path;
		SString vec_file_path;
		SString tok_file_path;

		//p_dict->GetDbSymb(temp_buf);
		/*{
			SString file_name;
			(file_name = "goodsnamelist").CatDiv('-', 0).Cat(temp_buf);
			PPGetFilePath(PPPATH_OUT, file_name, basis_path_wo_ext);
		}*/
		(basis_path_wo_ext = pPath).Strip().SetLastSlash().Cat(pBasisName);
		SFsPath::NormalizePath(temp_buf = basis_path_wo_ext, SFsPath::npfSlash, basis_path_wo_ext);

		(raw_input_file_path = basis_path_wo_ext).DotCat("txt");
		(model_file_path = basis_path_wo_ext).DotCat("model");
		(vec_file_path = basis_path_wo_ext).DotCat("vec");
		(tok_file_path = basis_path_wo_ext).DotCat("tok");
		/*{
			PPTextAnalyzer txta;
			txta.MakeGoodsNameList(raw_input_file_path);
		}*/
		{
			SString cmd_line;
			cmd_line.CatEq("--input", raw_input_file_path).Space().CatEq("--model_prefix", basis_path_wo_ext).Space();
			if(vocabSize > 0)
				cmd_line.CatEq("--vocab_size", vocabSize);
			//cmd_line..Space().CatEq("--input_sentence_size", 500000).Space().CatEq("--shuffle_input_sentence", "true")
			//sentencepiece::SentencePieceTrainer::Train("--input=test/botchan.txt --model_prefix=m --vocab_size=1000");
			absl::string_view sv_cmd_line(cmd_line);
			const auto status = sentencepiece::SentencePieceTrainer::Train(sv_cmd_line);
			if(status.ok()) {
				;
			}
			else {
				; // @todo @err
			}
		}
	}
	return ok;
}

int main(int argc, char * argv[])
{
	int r = BuildSentencePieceDictionary("C:\\ppy\\out", "goodsnamelist-uhtt", /*vocabSize*/8000);
	return 0;
}


