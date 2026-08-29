#include "stdafx.h"
#include <adplug.h>
#include <emuopl.h>
#include <kemuopl.h>
#include <nemuopl.h>
#include <temuopl.h>
#include <wemuopl.h>
#include <player.h>
#include <fprovide.h>
#include <binstr.h>
#include <surroundopl.h>
#include "GUID.h"
#include "global_config.h"

class MemStream : public binsstream {
private:
	std::vector<uint8_t> m_data;
public:
	explicit MemStream(std::vector<uint8_t>&& data) :
		binsbase(data.data(), (unsigned long)data.size()), binsstream(data.data(), (unsigned long)data.size()), m_data(std::move(data)) {
		setFlag(binio::BigEndian, false);
		setFlag(binio::FloatIEEE);
	}
};
class Fb2kFileProvider : public CFileProvider {
private:
	mutable service_ptr_t<file> p_file{nullptr};
	const char* p_path;
public:
	Fb2kFileProvider(const char* path) : p_path(path) {};
	binistream* open(std::string native_path) const override {
		size_t pos = native_path.find_last_of("\\/") + 1;
		std::string req_file_name = native_path.substr(pos);
		try {
			if (_stricmp(req_file_name.c_str(), fb2k::filename_ext(p_path)) != 0) {
				std::string track_file_path = p_path;
				size_t pos2 = track_file_path.find_last_of("\\/") + 1;
				std::string req_file_path = track_file_path.replace(pos2, std::string::npos, req_file_name);
				filesystem::g_open_read(p_file, req_file_path.c_str(), fb2k::noAbort);
			}
			else {
				filesystem::g_open_read(p_file, p_path, fb2k::noAbort);
			}
		}
		catch (...) {
			std::string error = "Can't find companion file: " + req_file_name;
			console::printf("[ADLIB] %s", error.c_str());
			return nullptr;
		}
		size_t file_size = p_file->get_size(fb2k::noAbort);
		std::vector<uint8_t> buffer(file_size);
		p_file->read_object(buffer.data(), file_size, fb2k::noAbort);
		MemStream* mem_stream = new MemStream(std::move(buffer));
		return mem_stream;
	}
	void close(binistream* stream) const override {
		if (stream) { delete stream; }
	}
};

class input_adplug : public input_stubs {
public:
	Copl* m_opl;
	CPlayer* m_player;
	service_ptr_t<file> m_file;
	static constexpr unsigned channels = 2;
	static constexpr unsigned bits_per_sample = 16;
	unsigned m_sample_rate = sample_rate_table[(int)cfg_adlib_samplerate];
	pfc::string8 m_song_type;
	unsigned m_remain_samples_until_next_event = 0;
	uint64_t m_played_samples = 0;
	uint64_t m_total_samples = 0;
	bool flag_eof = false;
	bool first_block = false;
	
public:
	~input_adplug() {
		delete m_opl;
		delete m_player;
	}
	//=======Helpers======
	Copl* make_core_base() {
		Copl* core;
		switch ((int)cfg_adlib_core) {
		case 0:
			core = new CWemuopl(m_sample_rate, true, true);
			break;
		case 1:
			core = new CKemuopl(m_sample_rate, true, true);
			break;
		case 2:
			core = new CEmuopl(m_sample_rate, true, true);
			break;
		case 3:
			core = new CTemuopl(m_sample_rate, true, true);
			break;
		case 4:
			core = new CNemuopl(m_sample_rate);
			break;
		default:
			uBugCheck();
			return nullptr;
		}
		return core;
	}
	Copl* make_core() {

		if ((bool)cfg_adlib_surround) {
			COPLprops a, b;
			a.opl = make_core_base();
			a.use16bit = true;
			a.stereo = true;
			b.opl = make_core_base();
			b.use16bit = true;
			b.stereo = true;
			auto* surrounded_core = new CSurroundopl(&a, &b, true);
			return surrounded_core;
		}
		else {
			return make_core_base();
		}
	}
	unsigned word_count(std::string& str) {
		unsigned count = 0;
		for (size_t i = 0; i < str.size();) {
			unsigned char byte = str[i];
			size_t length;
			if ((byte & 0b1000'0000) == 0) {
				length = 1;
			}
			else if ((byte & 0b1110'0000) == 0b1100'0000) {
				length = 2;
			}
			else if ((byte & 0b1111'0000) == 0b1110'0000) {
				length = 3;
			}
			else if ((byte & 0b1111'1000) == 0b1111'0000) {
				length = 4;
			}
			else {
				throw std::runtime_error("Invalid UTF-8 leading byte");
			}
			for (size_t j = 1; j < length; j++) {
				const auto continuation = static_cast<unsigned char>(str[i + j]);
				if ((continuation & 0b1100'0000) != 0b1000'0000) {
					throw std::runtime_error("Invalid UTF-8 continuation byte");
				}
			}
			i += length;
			count++;
		}
		return count;
	}
	std::string convert_DOS_to_utf8(const std::string& DOS_str) {
		std::string utf8_str{};
		for (unsigned i = 0; i < 16; i++) {
			pfc::stringcvt::string_utf8_from_codepage conv(DOS_codepage_table[i], DOS_str.c_str(), DOS_str.size());
			utf8_str = std::string(conv.get_ptr(), conv.length());
			unsigned word_number = word_count(utf8_str);
			if (word_number == DOS_str.size()) break;
		}
		return utf8_str;
	}
	std::string convert_Johab_to_utf8(const std::string& Johab_str) {
		std::string utf8_str{};
		pfc::stringcvt::string_utf8_from_codepage conv(1361, Johab_str.c_str(), Johab_str.size());
		utf8_str = std::string(conv.get_ptr(), conv.length());
		return utf8_str;
	}
	//=======Helpers End=======
	void open(service_ptr_t<file> p_filehint, const char* p_path, t_input_open_reason p_reason, abort_callback& p_abort) {
		if (p_reason == input_open_info_write) throw exception_tagging_unsupported();//our input does not support retagging.
		m_file = p_filehint;//p_filehint may be null, hence next line
		input_open_file_helper(m_file, p_path, p_reason, p_abort);//if m_file is null, opens file with appropriate privileges for our operation (read/write for writing tags, read-only otherwise).
		m_opl = make_core();
		pfc::string8 native_path;
		filesystem::g_get_native_path(p_path, native_path);
		char* type_from_factory;
		Fb2kFileProvider fp(p_path);
		m_player = CAdPlug::factory(native_path.c_str(), m_opl, type_from_factory, CAdPlug::players, fp);
		if (!m_player) throw exception_io_data();
		m_song_type = type_from_factory;
		if (_stricmp(m_song_type, "MIDI") == 0 || _stricmp(m_song_type, "Flash") == 0 || 
			_stricmp(m_song_type, "Hybrid") == 0 || _stricmp(m_song_type, "Hypnosis") == 0 || 
			_stricmp(m_song_type, "PSI") == 0 || _stricmp(m_song_type, "rat") == 0) {
			m_song_type = m_player->gettype().c_str();
		}
	}
	

	unsigned get_subsong_count() {
		return m_player->getsubsongs();
	}
	t_uint32 get_subsong(unsigned p_index) {
		return p_index;
	}
	void get_info(t_uint32 p_subsong, file_info& p_info, abort_callback& p_abort) {
		p_info.set_length(m_player->songlength(p_subsong) / 1000.0);
		//note that the values below should be based on contents of the file itself, NOT on user-configurable variables for an example. To report info that changes independently from file contents, use get_dynamic_info/get_dynamic_info_track instead.
		p_info.info_set_int("channels", channels);
		p_info.info_set_int("bitspersample", bits_per_sample);
		p_info.info_set("encoding", "synthesized");
		
		if (!m_song_type.isEmpty()) {
			p_info.info_set("codec", m_song_type);
		}
		std::string title = m_player->gettitle();
		if (!title.empty()) {
			std::string utf8_title{};
			if (_stricmp(m_song_type, "AdLib MIDI/IMS Format") == 0) {
				utf8_title = convert_Johab_to_utf8(title);
			}
			else {
				utf8_title = convert_DOS_to_utf8(title);
			}
			p_info.meta_set("title", utf8_title.c_str());
		}
		std::string author = m_player->getauthor();
		if (!author.empty()) {
			std::string utf8_author{};
			if (_stricmp(m_song_type, "AdLib MIDI/IMS Format") == 0) {
				utf8_author = convert_Johab_to_utf8(author);
			}
			else {
				utf8_author = convert_DOS_to_utf8(author);
			}
			p_info.meta_set("artist", utf8_author.c_str());
		}
		std::string desc = m_player->getdesc();
		if (!desc.empty()) {
			std::string utf8_desc{};
			if (_stricmp(m_song_type, "AdLib MIDI/IMS Format") == 0) {
				utf8_desc = convert_Johab_to_utf8(desc);
			}
			else {
				utf8_desc = convert_DOS_to_utf8(title);
			}
			p_info.meta_set("comment", utf8_desc.c_str());
		}

	}
	t_filestats2 get_stats2(unsigned f, abort_callback& p_abort) { return m_file->get_stats2_(f, p_abort); }
	t_filestats get_file_stats(abort_callback& p_abort) { return m_file->get_stats(p_abort); }

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback& p_abort) {
		first_block = true;
		m_player->rewind(p_subsong);
		flag_eof = false;
		m_total_samples = static_cast<uint64_t>(m_player->songlength(p_subsong)) * m_sample_rate / 1000;
		m_played_samples = 0;
		m_remain_samples_until_next_event = 0;
	}
	bool decode_run(audio_chunk& p_chunk, abort_callback& p_abort) {
		enum { chunk_size = 1024 };
		if (flag_eof) return false;
		int16_t buffer[chunk_size * channels];
		unsigned chunk_samples = (m_total_samples - m_played_samples < chunk_size) ? m_total_samples - m_played_samples : chunk_size;
		unsigned produced_samples = 0;
		while (produced_samples < chunk_samples) {
			unsigned chunk_needed = chunk_samples - produced_samples;
			if (m_remain_samples_until_next_event > 0) {
				unsigned samples_to_render = chunk_needed < m_remain_samples_until_next_event ? chunk_needed : m_remain_samples_until_next_event;
				m_opl->update(buffer + produced_samples * channels, samples_to_render);
				m_remain_samples_until_next_event -= samples_to_render;
				produced_samples += samples_to_render;
			}
			else if (m_player->update()) {
				float fresh_freq = m_player->getrefresh();
				m_remain_samples_until_next_event = m_sample_rate / fresh_freq;
			}
			else {
				flag_eof = true;
				break;
			}
		}
		if (produced_samples == 0) return false;
		m_played_samples += produced_samples;
		p_chunk.set_data_fixedpoint(buffer, produced_samples * channels * (bits_per_sample / 8), m_sample_rate, channels, bits_per_sample, audio_chunk::g_guess_channel_config(channels));
		return true;
		
	}
	void decode_seek(double p_seconds, abort_callback& p_abort) {
		first_block = true;
		unsigned long p_ms = static_cast<unsigned long>(p_seconds * 1000);
		m_played_samples = static_cast<uint64_t>(p_ms) * m_sample_rate / 1000;
		m_player->seek(p_ms);
		float fresh_freq = m_player->getrefresh();
		m_remain_samples_until_next_event = static_cast<unsigned>(m_sample_rate / fresh_freq);
	}
	bool decode_can_seek() { return true; }
	bool decode_get_dynamic_info(file_info& p_out, double& p_timestamp_delta) {
		if (first_block) {
			p_out.info_set_int("samplerate", m_sample_rate);
			first_block = false;
			return true;
		}
		else {
			return false;
		}
	}
	bool decode_get_dynamic_info_track(file_info& p_out, double& p_timestamp_delta) { return false; } // deals with dynamic information such as track changes in live streams
	void decode_on_idle(abort_callback& p_abort) { m_file->on_idle(p_abort); }

	// Note that open() already rejects requests to open for tag writing, so these should never get called.
	void retag_set_info(t_uint32 p_subsong, const file_info& p_info, abort_callback& p_abort) { throw exception_tagging_unsupported(); }
	void retag_commit(abort_callback& p_abort) { throw exception_tagging_unsupported(); }
	void remove_tags(abort_callback&) { throw exception_tagging_unsupported(); }

	static bool g_is_our_content_type(const char* p_content_type) { return false; } // match against supported mime types here
	static bool g_is_our_path(const char* p_path, const char* p_extension) {
		for (auto& player : CAdPlug::players) {
			for (int i = 0; player->get_extension(i); i++) {
				const char* ext = player->get_extension(i) + 1; //skip"."
				if (_stricmp(p_extension, ext) == 0 && 
					_stricmp(p_extension, "mid") != 0 && _stricmp(p_extension, "s3m") != 0) return true;
					// for general MIDI file, foo_midi is better; for Scream Tracker 3, foo_openmpt54 is better
			}
		}
		return false;
	}
	static const char* g_get_name() { return "Adlib OPL Decoder"; }
	static const GUID g_get_guid() { return guid_decoder; }
};

static input_factory_t<input_adplug> g_input_adplug_factory;

