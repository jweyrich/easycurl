#pragma once

#include <curl/curl.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>
#include "cxx-prettyprint/prettyprint98.hpp"
#include "ios_state.h"

class easycurl_util {
public:
	// static std::string char2hex(char dec);
	// static std::string url_encode(const std::string& c);
	static std::string escape(const std::string& value);
	static std::string unescape(const std::string& value);
private:
	easycurl_util();
	easycurl_util(const easycurl_util&);
	easycurl_util& operator=(const easycurl_util&);
};

std::string easycurl_util::escape(const std::string& value) {
	CURL *curl = curl_easy_init();
	char *escaped = curl_easy_escape(curl, value.c_str(), value.length());
	std::string result = escaped;
	curl_free(static_cast<void *>(escaped));
	curl_easy_cleanup(curl);
	return result;
}

std::string easycurl_util::unescape(const std::string& value) {
	CURL *curl = curl_easy_init();
	char *unescaped = curl_easy_unescape(curl, value.c_str(), value.length(), NULL);
	std::string result = unescaped;
	curl_free(static_cast<void *>(unescaped));
	curl_easy_cleanup(curl);
	return result;
}

class easycurl_payload {
public:
	typedef std::string						key_type;
	typedef std::string						value_type;
	typedef std::pair<key_type, value_type>	pair_type;
	typedef std::map<key_type, value_type>	data_type;

	easycurl_payload() {}
	easycurl_payload& add(const key_type& key, const value_type& value) {
		_data.insert(pair_type(key, value));
		return *this;
	}
	easycurl_payload& add_all(const data_type& data) {
		_data.insert(data.begin(), data.end());
		return *this;
	}
	easycurl_payload& del(const key_type& key) {
		_data.erase(key);
		return *this;
	}
	friend std::ostream& operator<<(std::ostream& stream, const easycurl_payload& payload) {
		stream << payload._data << std::endl;
// 		for (data_type::const_iterator it = payload._data.begin(); it != payload._data.end(); ++it) {
// 			stream << it->first << ": " << it->second << std::endl;
// 		}
		return stream;
	}
	const data_type& data() const {
		return _data;
	}
	std::string to_string() const {
		std::string result = "";

		data_type::const_iterator curr, end;
		data_type::const_reverse_iterator last;
		for (curr = _data.begin(), end = _data.end(), last = _data.rbegin(); curr != end; ++curr) {
			result += curr->first + "=" + curr->second + (&*curr != &*last ? "&" : "");
		}

		return result;
	}
protected:
	data_type _data;
private:
	easycurl_payload(const easycurl_payload&);
	easycurl_payload& operator=(const easycurl_payload&);
};

struct easycurl_debug_config {
public:
	easycurl_debug_config(std::ostream& stream)
		: stream(&stream)
		, show_hex(false)
	{}
	std::ostream *stream;
	bool show_hex;
private:
	easycurl_debug_config(const easycurl_debug_config&);
	easycurl_debug_config& operator=(const easycurl_debug_config&);
};

static void easycurl_dump(
	struct easycurl_debug_config *debug_config,
	const char *text,
	unsigned char *ptr, size_t size)
{
	const bool nohex = !debug_config->show_hex;
	size_t i;
	size_t c;
	// Without the hex output, we can fit more on screen
	const unsigned int width = nohex ? 0x50 : 0x10;

	// fprintf(debug_config->stream, "%s, %10.10ld bytes\n", text, (long)size);
	// fprintf(debug_config->stream, "%s", text);
	*debug_config->stream << text;

	for (i=0; i<size; i+= width) {
		// fprintf(debug_config->stream, "%4.4lx: ", (long)i);
		if (!nohex) {
			// Hex not disabled, show it
			for (c = 0; c < width; c++) {
				if (i+c < size) {
					// fprintf(debug_config->stream, "%02x ", ptr[i+c]);
					ios_state_saver saver(*debug_config->stream);
					*debug_config->stream
						<< std::hex
						<< std::setprecision(2)
						<< std::setfill('0')
						<< std::setw(2)
						<< static_cast<int>(ptr[i+c]);
					// Reset
					saver.restore();
					*debug_config->stream << ' ';
				} else {
					// Padding for last line, e.g.:
					// 3b 77 68 69 6c 65 28 61 26 26 21 28 61 2e 67 65 ;while(a&&!(a.ge
					// 74 41 74 74 72 69 62 75 74 65                   tAttribute
					// fputs("   ", debug_config->stream);
					*debug_config->stream << "   ";
				}
			}
		}

		for (c = 0; (c < width) && (i+c < size); c++) {
			// Check for 0D0A; if found, skip past and start a new line of output
			if (nohex && (i+c+1 < size) && ptr[i+c] == 0x0D && ptr[i+c+1] == 0x0A) {
				i += (c + 2 - width);
				break;
			}
			const char digit = (ptr[i+c] >= 0x20) && (ptr[i+c] < 0x80) ? ptr[i+c] : '.';
			// fprintf(debug_config->stream, "%c", digit);
			*debug_config->stream << digit;

			// Check again for 0D0A, to avoid an extra \n if it's at width
			if (nohex && (i+c+2 < size) && ptr[i+c+1] == 0x0D && ptr[i+c+2] == 0x0A) {
				i += (c+3-width);
				break;
			}
		}
		// fputc('\n', debug_config->stream); // Newline
		*debug_config->stream << '\n';
	}
	// fflush(debug_config->stream);
	debug_config->stream->flush();
}

static int easycurl_trace(
	CURL *handle, curl_infotype type,
	char *data, size_t size,
	void *userp)
{
	struct easycurl_debug_config *debug_config = (struct easycurl_debug_config *)userp;
	const char *text;
	(void)handle; // prevent compiler warning

	switch (type) {
		case CURLINFO_TEXT:
			// fprintf(stderr, "== Info: %s", data);
			*debug_config->stream << "== Info: " << data;
		default: // In case a new one is introduced to shock us
			return 0;
		case CURLINFO_HEADER_OUT:
			// text = "=> Send header";
			text = "=> ";
			break;
		case CURLINFO_DATA_OUT:
			text = "=> Send data:\n";
			break;
		case CURLINFO_SSL_DATA_OUT:
			// text = "=> Send SSL data";
			return 0;
		case CURLINFO_HEADER_IN:
			// text = "<= Recv header";
			text = "<= ";
			break;
		case CURLINFO_DATA_IN:
			text = "<= Recv data:\n";
			break;
		case CURLINFO_SSL_DATA_IN:
			// text = "<= Recv SSL data";
			return 0;
	}

	easycurl_dump(debug_config, text, (unsigned char *)data, size);
	return 0;
}

class easycurl_session {
public:
	easycurl_session()
		: _debug_config(std::cerr)
	{
		_curl = curl_easy_init();
		if (_curl == NULL)
			std::abort();
		use_default_setup();
	}
	~easycurl_session() {
		curl_easy_cleanup(_curl);
	}
	std::string get(const std::string& url);
	std::string post(const std::string& url, const easycurl_payload& payload);
	std::string post_multipart(const std::string& url, const easycurl_payload& payload);
public:
	void reset() {
		_user_agent = "";
		_cookie_file = "";
		disable_debug();
		curl_easy_reset(_curl);
	}
	void use_default_setup() {
		reset();
		curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error_buffer);
		set_user_agent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_9_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/34.0.1828.0 Safari/537.36");
		set_cookie_file("cookies.txt");

		// Follow any "Location:"
		curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt(_curl, CURLOPT_MAXREDIRS, 10);
		// Automatically add "Referer:" in requests after following a "Location:" redirect.
		curl_easy_setopt(_curl, CURLOPT_AUTOREFERER, 1);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &easycurl_session::static_writer);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 0);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, 2);

		// curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, 1);
		// curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	}
	void enable_debug(std::ostream *debug_stream = &std::cerr) {
		curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(_curl, CURLOPT_HEADER, 0);
		_debug_config.stream = debug_stream;
		curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, easycurl_trace);
		curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, &_debug_config);
	}
	void disable_debug() {
		curl_easy_setopt(_curl, CURLOPT_VERBOSE, 0);
		curl_easy_setopt(_curl, CURLOPT_HEADER, 0);
	}
	void set_user_agent(const std::string& user_agent) {
		_user_agent = user_agent;
		curl_easy_setopt(_curl, CURLOPT_USERAGENT, _user_agent.c_str());
	}
	void set_cookie_file(const std::string& file) {
		_cookie_file = file;
		curl_easy_setopt(_curl, CURLOPT_COOKIEFILE, _cookie_file.c_str()); // Read from
		curl_easy_setopt(_curl, CURLOPT_COOKIEJAR, _cookie_file.c_str()); // Write to
	}
protected:
	CURL *_curl;
	struct easycurl_debug_config _debug_config;
	std::string _user_agent;
	std::string _cookie_file;
	char _error_buffer[CURL_ERROR_SIZE];
	std::string _buffer;

	// f is the pointer to your object.
	static int static_writer(char *data, size_t size, size_t nmemb, void *object) {
		// Call non-static member function.
		easycurl_session *instance = static_cast<easycurl_session *>(object);
		return instance->writer(data, size, nmemb, instance->_buffer);
	}
	int writer(const char *data, size_t size, size_t nmemb, std::string& buffer);
	std::string easycurl(const std::string& url, const easycurl_payload *payload = NULL, bool is_multipart = false);

private:
	easycurl_session(const easycurl_session&);
	easycurl_session& operator=(const easycurl_session&);
};

int easycurl_session::writer(const char *data, size_t size, size_t nmemb, std::string& buffer) {
	buffer.append(data, size * nmemb);
	return size * nmemb;
}

std::string easycurl_session::get(const std::string& url) {
	return easycurl(url, NULL);
}

std::string easycurl_session::post(const std::string& url, const easycurl_payload& payload) {
	bool is_multipart = false;
	return easycurl(url, &payload, is_multipart);
}

std::string easycurl_session::post_multipart(const std::string& url, const easycurl_payload& payload) {
	bool is_multipart = true;
	return easycurl(url, &payload, is_multipart);
}

std::string easycurl_session::easycurl(const std::string& url, const easycurl_payload *payload, bool is_multipart) {
	// Reset members
	_buffer = "";
	_error_buffer[0] = '\0';

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());

	std::string data; // Keep it in scope during all the operation
	struct curl_httppost *formpost = NULL;

	if (payload != NULL) {
		if (is_multipart) {
			// std::cout << "### POSTING MULTIPART: " << payload->to_string() << std::endl;
			struct curl_httppost *lastptr = NULL;
			easycurl_payload::data_type::const_iterator current;
			easycurl_payload::data_type::const_iterator end;
			for (current = payload->data().begin(), end = payload->data().end(); current != end; ++current) {
				curl_formadd(&formpost,
					&lastptr,
					CURLFORM_COPYNAME, current->first.c_str(),
					CURLFORM_COPYCONTENTS, current->second.c_str(),
					CURLFORM_END);
			}
			curl_easy_setopt(_curl, CURLOPT_HTTPPOST, formpost);
		} else {
			data = payload->to_string();
			// std::cout << "### POSTING: " << data << std::endl;
			curl_easy_setopt(_curl, CURLOPT_POST, 1);
			curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, data.c_str());
			curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE, data.length());
		}
	}

	// Execute
	CURLcode result = curl_easy_perform(_curl);

	if (payload != NULL && is_multipart) {
		// Cleanup the formpost chain
		curl_formfree(formpost);
	}

	// Did we succeed?
	if (result != CURLE_OK) {
		std::cerr << "Error: " << result << ": " << curl_easy_strerror(result) << std::endl;
		// std::cerr << "Error:" << error_buffer << std::endl;
		return "";
	}

	return _buffer;
}
