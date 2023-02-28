// Copyright 2018 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.!
//
#include <sentencepiece-internal.h>
#pragma hdrstop
//
// spec_parser.h
//
namespace sentencepiece {
	#define PARSE_STRING(param_name)                   \
		if(name == #param_name) {                       \
			message->set_ ## param_name(std::string(value)); \
			return util::OkStatus();                       \
		}

	#define PARSE_REPEATED_STRING(param_name)                       \
		if(name == #param_name) {                                    \
			for(const std::string &val : util::StrSplitAsCSV(value)) { \
				message->add_ ## param_name(val);                           \
			}                                                           \
			return util::OkStatus();                                    \
		}

	#define PARSE_BYTE(param_name)                             \
		if(name == #param_name) {                               \
			message->set_ ## param_name(value.data(), value.size()); \
			return util::OkStatus();                               \
		}

	#define PARSE_INT32(param_name)                                               \
		if(name == #param_name) {                                                  \
			int32 v;                                                                  \
			if(!string_util::lexical_cast(value, &v))                                \
				return util::StatusBuilder(util::StatusCode::kInvalidArgument, GTL_LOC) << "cannot parse \"" << value << "\" as int."; \
			message->set_ ## param_name(v);                                             \
			return util::OkStatus();                                                  \
		}

	#define PARSE_UINT64(param_name)                                              \
		if(name == #param_name) {                                                  \
			uint64 v;                                                                 \
			if(!string_util::lexical_cast(value, &v))                                \
				return util::StatusBuilder(util::StatusCode::kInvalidArgument, GTL_LOC) << "cannot parse \"" << value << "\" as int."; \
			message->set_ ## param_name(v);                                             \
			return util::OkStatus();                                                  \
		}

	#define PARSE_DOUBLE(param_name)                                              \
		if(name == #param_name) {                                                  \
			double v;                                                                 \
			if(!string_util::lexical_cast(value, &v))                                \
			return util::StatusBuilder(util::StatusCode::kInvalidArgument, GTL_LOC) \
				<< "cannot parse \"" << value << "\" as int.";                   \
			message->set_ ## param_name(v);                                             \
			return util::OkStatus();                                                  \
		}

	#define PARSE_BOOL(param_name)                                                \
		if(name == #param_name) {                                                  \
			bool v;                                                                   \
			if(!string_util::lexical_cast(value.empty() ? "true" : value, &v))       \
			return util::StatusBuilder(util::StatusCode::kInvalidArgument, GTL_LOC) \
				<< "cannot parse \"" << value << "\" as bool.";                  \
			message->set_ ## param_name(v);                                             \
			return util::OkStatus();                                                  \
		}

	#define PARSE_ENUM(param_name, map_name)                                      \
		if(name == #param_name) {                                                  \
			const auto it = map_name.find(absl::AsciiStrToUpper(value));              \
			if(it == map_name.end())                                                 \
			return util::StatusBuilder(util::StatusCode::kInvalidArgument, GTL_LOC) \
				<< "unknown enumeration value of \"" << value << "\" as "        \
				<< #map_name;                                                    \
			message->set_ ## param_name(it->second);                                    \
			return util::OkStatus();                                                  \
		}

	#define PRINT_PARAM(param_name) \
		os << "  " << #param_name << ": " << message.param_name() << "\n";

	#define PRINT_REPEATED_STRING(param_name)    \
		for(const auto &v : message.param_name()) \
		os << "  " << #param_name << ": " << v << "\n";

	#define PRINT_ENUM(param_name, map_name)               \
		const auto it = map_name.find(message.param_name()); \
		if(it == map_name.end())                            \
		os << "  " << #param_name << ": unknown\n";        \
		else                                                 \
		os << "  " << #param_name << ": " << it->second << "\n";

	inline std::string PrintProto(const TrainerSpec &message, absl::string_view name) 
	{
		std::ostringstream os;
		os << name << " {\n";
		PRINT_REPEATED_STRING(input);
		PRINT_PARAM(input_format);
		PRINT_PARAM(model_prefix);
		static const std::map<TrainerSpec::ModelType, std::string> kModelType_Map = {
			{TrainerSpec::UNIGRAM, "UNIGRAM"}, {TrainerSpec::BPE, "BPE"}, {TrainerSpec::WORD, "WORD"}, {TrainerSpec::CHAR, "CHAR"},
		};

		PRINT_ENUM(model_type, kModelType_Map);
		PRINT_PARAM(vocab_size);
		PRINT_REPEATED_STRING(accept_language);
		PRINT_PARAM(self_test_sample_size);
		PRINT_PARAM(character_coverage);
		PRINT_PARAM(input_sentence_size);
		PRINT_PARAM(shuffle_input_sentence);
		PRINT_PARAM(seed_sentencepiece_size);
		PRINT_PARAM(shrinking_factor);
		PRINT_PARAM(max_sentence_length);
		PRINT_PARAM(num_threads);
		PRINT_PARAM(num_sub_iterations);
		PRINT_PARAM(max_sentencepiece_length);
		PRINT_PARAM(split_by_unicode_script);
		PRINT_PARAM(split_by_number);
		PRINT_PARAM(split_by_whitespace);
		PRINT_PARAM(split_digits);
		PRINT_PARAM(treat_whitespace_as_suffix);
		PRINT_PARAM(allow_whitespace_only_pieces);
		PRINT_REPEATED_STRING(control_symbols);
		PRINT_REPEATED_STRING(user_defined_symbols);
		PRINT_PARAM(required_chars);
		PRINT_PARAM(byte_fallback);
		PRINT_PARAM(vocabulary_output_piece_score);
		PRINT_PARAM(train_extremely_large_corpus);
		PRINT_PARAM(hard_vocab_limit);
		PRINT_PARAM(use_all_vocab);
		PRINT_PARAM(unk_id);
		PRINT_PARAM(bos_id);
		PRINT_PARAM(eos_id);
		PRINT_PARAM(pad_id);
		PRINT_PARAM(unk_piece);
		PRINT_PARAM(bos_piece);
		PRINT_PARAM(eos_piece);
		PRINT_PARAM(pad_piece);
		PRINT_PARAM(unk_surface);
		PRINT_PARAM(enable_differential_privacy);
		PRINT_PARAM(differential_privacy_noise_level);
		PRINT_PARAM(differential_privacy_clipping_threshold);
		os << "}\n";
		return os.str();
	}

	inline std::string PrintProto(const NormalizerSpec &message, absl::string_view name) 
	{
		std::ostringstream os;
		os << name << " {\n";
		PRINT_PARAM(name);
		PRINT_PARAM(add_dummy_prefix);
		PRINT_PARAM(remove_extra_whitespaces);
		PRINT_PARAM(escape_whitespaces);
		PRINT_PARAM(normalization_rule_tsv);
		os << "}\n";
		return os.str();
	}

	util::Status SentencePieceTrainer::SetProtoField(absl::string_view name, absl::string_view value, TrainerSpec * message) 
	{
		CHECK_OR_RETURN(message);
		PARSE_REPEATED_STRING(input);
		PARSE_STRING(input_format);
		PARSE_STRING(model_prefix);
		static const std::map<std::string, TrainerSpec::ModelType> kModelType_Map = { 
			{"UNIGRAM", TrainerSpec::UNIGRAM}, {"BPE", TrainerSpec::BPE}, {"WORD", TrainerSpec::WORD}, {"CHAR", TrainerSpec::CHAR}, 
		};
		PARSE_ENUM(model_type, kModelType_Map);
		PARSE_INT32(vocab_size);
		PARSE_REPEATED_STRING(accept_language);
		PARSE_INT32(self_test_sample_size);
		PARSE_DOUBLE(character_coverage);
		PARSE_UINT64(input_sentence_size);
		PARSE_BOOL(shuffle_input_sentence);
		PARSE_INT32(seed_sentencepiece_size);
		PARSE_DOUBLE(shrinking_factor);
		PARSE_INT32(max_sentence_length);
		PARSE_INT32(num_threads);
		PARSE_INT32(num_sub_iterations);
		PARSE_INT32(max_sentencepiece_length);
		PARSE_BOOL(split_by_unicode_script);
		PARSE_BOOL(split_by_number);
		PARSE_BOOL(split_by_whitespace);
		PARSE_BOOL(split_digits);
		PARSE_BOOL(treat_whitespace_as_suffix);
		PARSE_BOOL(allow_whitespace_only_pieces);
		PARSE_REPEATED_STRING(control_symbols);
		PARSE_REPEATED_STRING(user_defined_symbols);
		PARSE_STRING(required_chars);
		PARSE_BOOL(byte_fallback);
		PARSE_BOOL(hard_vocab_limit);
		PARSE_BOOL(vocabulary_output_piece_score);
		PARSE_BOOL(train_extremely_large_corpus);
		PARSE_BOOL(use_all_vocab);
		PARSE_INT32(unk_id);
		PARSE_INT32(bos_id);
		PARSE_INT32(eos_id);
		PARSE_INT32(pad_id);
		PARSE_STRING(unk_piece);
		PARSE_STRING(bos_piece);
		PARSE_STRING(eos_piece);
		PARSE_STRING(pad_piece);
		PARSE_STRING(unk_surface);
		PARSE_BOOL(enable_differential_privacy);
		PARSE_DOUBLE(differential_privacy_noise_level);
		PARSE_UINT64(differential_privacy_clipping_threshold);
		return util::StatusBuilder(util::StatusCode::kNotFound, GTL_LOC) << "unknown field name \"" << name << "\" in TrainerSpec.";
	}

	util::Status SentencePieceTrainer::SetProtoField(absl::string_view name, absl::string_view value, NormalizerSpec * message) 
	{
		CHECK_OR_RETURN(message);
		PARSE_STRING(name);
		PARSE_BYTE(precompiled_charsmap);
		PARSE_BOOL(add_dummy_prefix);
		PARSE_BOOL(remove_extra_whitespaces);
		PARSE_BOOL(escape_whitespaces);
		PARSE_STRING(normalization_rule_tsv);
		return util::StatusBuilder(util::StatusCode::kNotFound, GTL_LOC) << "unknown field name \"" << name << "\" in NormalizerSpec.";
	}

	#undef PARSE_STRING
	#undef PARSE_REPEATED_STRING
	#undef PARSE_BOOL
	#undef PARSE_BYTE
	#undef PARSE_INT32
	#undef PARSE_DUOBLE
	#undef PARSE_ENUM
	#undef PRINT_MAP
	#undef PRINT_REPEATED_STRING
	#undef PRINT_ENUM
}  // namespace sentencepiece

namespace sentencepiece {
	namespace {
		static constexpr char kDefaultNormalizerName[] = "nmt_nfkc";
	}

	/*static*/util::Status SentencePieceTrainer::Train(const TrainerSpec &trainer_spec, SentenceIterator * sentence_iterator, std::string * serialized_model_proto) 
	{
		NormalizerSpec normalizer_spec;
		return Train(trainer_spec, normalizer_spec, sentence_iterator, serialized_model_proto);
	}

	util::Status SentencePieceTrainer::Train(const TrainerSpec &trainer_spec, const NormalizerSpec &normalizer_spec, SentenceIterator * sentence_iterator, std::string * serialized_model_proto) 
	{
		NormalizerSpec denormalizer_spec;
		return Train(trainer_spec, normalizer_spec, denormalizer_spec, sentence_iterator, serialized_model_proto);
	}

	/*static*/util::Status SentencePieceTrainer::Train(const TrainerSpec &trainer_spec, const NormalizerSpec &normalizer_spec,
		const NormalizerSpec &denormalizer_spec, SentenceIterator * sentence_iterator, std::string * serialized_model_proto) 
	{
		auto copied_normalizer_spec = normalizer_spec;
		RETURN_IF_ERROR(PopulateNormalizerSpec(&copied_normalizer_spec, false));
		auto copied_denormalizer_spec = denormalizer_spec;
		RETURN_IF_ERROR(PopulateNormalizerSpec(&copied_denormalizer_spec, true));
		auto trainer = TrainerFactory::Create(trainer_spec, copied_normalizer_spec, copied_denormalizer_spec);
		std::string info = absl::StrCat(PrintProto(trainer_spec, "trainer_spec"), PrintProto(copied_normalizer_spec, "normalizer_spec"));
		if(!copied_denormalizer_spec.precompiled_charsmap().empty()) {
			info += PrintProto(copied_denormalizer_spec, "denormalizer_spec");
		}
		else {
			info += "denormalizer_spec {}";
		}
		LOG(INFO) << "Starts training with : \n" << info;
		if(serialized_model_proto) {
			ModelProto model_proto;
			RETURN_IF_ERROR(trainer->Train(sentence_iterator, &model_proto));
			*serialized_model_proto = model_proto.SerializeAsString();
		}
		else {
			RETURN_IF_ERROR(trainer->Train(sentence_iterator, nullptr));
		}
		return util::OkStatus();
	}

	/*static*/NormalizerSpec SentencePieceTrainer::GetNormalizerSpec(absl::string_view name) 
	{
		NormalizerSpec spec;
		spec.set_name(name.data(), name.size());
		CHECK_OK(normalizer::Builder::GetPrecompiledCharsMap(spec.name(), spec.mutable_precompiled_charsmap()));
		return spec;
	}

	/*static*/util::Status SentencePieceTrainer::MergeSpecsFromArgs(absl::string_view args, TrainerSpec * trainer_spec, NormalizerSpec * normalizer_spec, NormalizerSpec * denormalizer_spec) 
	{
		CHECK_OR_RETURN(trainer_spec) << "`trainer_spec` must not be null.";
		CHECK_OR_RETURN(normalizer_spec) << "`normalizer_spec` must not be null.";
		CHECK_OR_RETURN(denormalizer_spec) << "`denormalizer_spec` must not be null.";
		if(args.empty()) 
			return util::OkStatus();
		std::unordered_map<std::string, std::string> kwargs;
		for(auto arg : absl::StrSplit(args, " ")) {
			absl::ConsumePrefix(&arg, "--");
			std::string key, value;
			const auto pos = arg.find('=');
			if(pos == absl::string_view::npos) {
				key = std::string(arg);
			}
			else {
				key = std::string(arg.substr(0, pos));
				value = std::string(arg.substr(pos + 1));
			}
			kwargs.emplace(key, value);
		}

		return MergeSpecsFromArgs(kwargs, trainer_spec, normalizer_spec,
			   denormalizer_spec);
	}

	/*static*/util::Status SentencePieceTrainer::MergeSpecsFromArgs(const std::unordered_map<std::string, std::string> &kwargs,
		TrainerSpec * trainer_spec, NormalizerSpec * normalizer_spec, NormalizerSpec * denormalizer_spec) 
	{
		CHECK_OR_RETURN(trainer_spec) << "`trainer_spec` must not be null.";
		CHECK_OR_RETURN(normalizer_spec) << "`normalizer_spec` must not be null.";
		CHECK_OR_RETURN(denormalizer_spec) << "`denormalizer_spec` must not be null.";

		for(const auto &it : kwargs) {
			const auto &key = it.first;
			const auto &value = it.second;
			// Exceptions.
			if(key == "normalization_rule_name") {
				normalizer_spec->set_name(value);
				continue;
			}
			else if(key == "denormalization_rule_tsv") {
				denormalizer_spec->set_normalization_rule_tsv(value);
				denormalizer_spec->set_add_dummy_prefix(false);
				denormalizer_spec->set_remove_extra_whitespaces(false);
				denormalizer_spec->set_escape_whitespaces(false);
				continue;
			}
			else if(key == "minloglevel") {
				int v = 0;
				CHECK_OR_RETURN(absl::SimpleAtoi(value, &v));
				logging::SetMinLogLevel(v);
				continue;
			}

			const auto status_train = SetProtoField(key, value, trainer_spec);
			if(status_train.ok()) continue;
			if(!util::IsNotFound(status_train)) return status_train;

			const auto status_norm = SetProtoField(key, value, normalizer_spec);
			if(status_norm.ok()) continue;
			if(!util::IsNotFound(status_norm)) return status_norm;

			// Not found both in trainer_spec and normalizer_spec.
			if(util::IsNotFound(status_train) && util::IsNotFound(status_norm)) {
				return status_train;
			}
		}
		return util::OkStatus();
	}

	/*static*/util::Status SentencePieceTrainer::Train(absl::string_view args, SentenceIterator * sentence_iterator, std::string * serialized_model_proto) 
	{
		LOG(INFO) << "Running command: " << args.data();
		TrainerSpec trainer_spec;
		NormalizerSpec normalizer_spec;
		NormalizerSpec denormalizer_spec;
		RETURN_IF_ERROR(MergeSpecsFromArgs(args, &trainer_spec, &normalizer_spec, &denormalizer_spec));
		return Train(trainer_spec, normalizer_spec, denormalizer_spec, sentence_iterator, serialized_model_proto);
	}

	/*static*/util::Status SentencePieceTrainer::Train(const std::unordered_map<std::string, std::string> &kwargs, SentenceIterator * sentence_iterator, std::string * serialized_model_proto) 
	{
		TrainerSpec trainer_spec;
		NormalizerSpec normalizer_spec;
		NormalizerSpec denormalizer_spec;
		RETURN_IF_ERROR(MergeSpecsFromArgs(kwargs, &trainer_spec, &normalizer_spec, &denormalizer_spec));
		return Train(trainer_spec, normalizer_spec, denormalizer_spec, sentence_iterator, serialized_model_proto);
	}

	/*static*/util::Status SentencePieceTrainer::PopulateNormalizerSpec(NormalizerSpec * normalizer_spec, bool is_denormalizer) 
	{
		CHECK_OR_RETURN(normalizer_spec);
		if(!normalizer_spec->normalization_rule_tsv().empty()) {
			CHECK_OR_RETURN(normalizer_spec->precompiled_charsmap().empty()) << "precompiled_charsmap is already defined.";
			normalizer::Builder::CharsMap chars_map;
			RETURN_IF_ERROR(normalizer::Builder::LoadCharsMap(normalizer_spec->normalization_rule_tsv(), &chars_map));
			RETURN_IF_ERROR(normalizer::Builder::CompileCharsMap(chars_map, normalizer_spec->mutable_precompiled_charsmap()));
			normalizer_spec->set_name("user_defined");
		}
		else if(!is_denormalizer) {
			if(normalizer_spec->name().empty()) {
				normalizer_spec->set_name(kDefaultNormalizerName);
			}
			if(normalizer_spec->precompiled_charsmap().empty()) {
				RETURN_IF_ERROR(normalizer::Builder::GetPrecompiledCharsMap(
						normalizer_spec->name(),
						normalizer_spec->mutable_precompiled_charsmap()));
			}
		}
		return util::OkStatus();
	}

	/*static*/util::Status SentencePieceTrainer::PopulateModelTypeFromString(absl::string_view type, TrainerSpec * spec) 
	{
		static const std::unordered_map<std::string, TrainerSpec::ModelType>
		kModelTypeMap = {{"unigram", TrainerSpec::UNIGRAM}, {"bpe", TrainerSpec::BPE}, {"word", TrainerSpec::WORD}, {"char", TrainerSpec::CHAR}};
		const auto it = kModelTypeMap.find(absl::AsciiStrToLower(type));
		if(it != kModelTypeMap.end()) {
			spec->set_model_type(it->second);
			return util::OkStatus();
		}
		return util::StatusBuilder(util::StatusCode::kInternal, GTL_LOC) << "\"" << type << "\" is not found in TrainerSpec";
	}

	namespace {
		const pretokenizer::PretokenizerForTrainingInterface * g_pretokenizer = nullptr;
	}

	/*static*/util::Status SentencePieceTrainer::SetPretokenizerForTraining(const pretokenizer::PretokenizerForTrainingInterface * pretokenizer) 
	{
		g_pretokenizer = pretokenizer;
		return util::OkStatus();
	}

	/*static*/const pretokenizer::PretokenizerForTrainingInterface * SentencePieceTrainer::GetPretokenizerForTraining() 
	{
		return g_pretokenizer;
	}
}  // namespace sentencepiece
