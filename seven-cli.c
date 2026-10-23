#include <cctype>
#include <iomanip>
#include <curl/curl.h>
#include <time.h>

#include <zlib.h>

#include <openssl/md5.h>
#include <openssl/engine.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <fstream>
#include <sstream>
#include <unistd.h>
#include <iostream>
#include <string>
#include <getopt.h>
#include "minini_12b/minIni.h"

using namespace std;

const char inifile[] = "config.ini";

string url_encode(const string &value) {
	ostringstream escaped;
	escaped.fill('0');
	escaped << hex;

	for (string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << '%' << setw(2) << int((unsigned char) c);
	}

	return escaped.str();
}

// callback function writes data to a ostream
static size_t data_write(void* buf, size_t size, size_t nmemb, void* userp)
{
	if(userp)
	{
		ostream& os = *static_cast<ostream*>(userp);
		streamsize len = size * nmemb;
		if(os.write(static_cast<char*>(buf), len))
			return len;
	}

	return 0;
}

/**
 * timeout is in seconds
 **/
CURLcode curl_read(const string& url, const string& data, ostream& os, long timeout = 600)
{
	CURLcode code(CURLE_FAILED_INIT);
	CURL* curl = curl_easy_init();

	if(curl)
	{
		if(
      CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &data_write))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
  		//&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FILE, &os))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
  		&& CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str()))
      && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_POST, 1L))
      && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str()))
    )
		{
			code = curl_easy_perform(curl);
		}
		else {
			cout << "CURL ERROR" << endl;
		}
		curl_easy_cleanup(curl);
	}
	return code;
}

// Print Usage
void usage(char* argv[]) {
	string app_filename = argv[0];

	string usage_info = "Usage:\n%s", separators;
	int i;
	for(i = 0; i < app_filename.size() + 1; i++) {
		separators += " ";
	}
	usage_info += " --apikey SEVEN_API_KEY\n"
		+ separators
		+ "--to Recipient list or file containing recipients\n"
		+ separators
		+ "--text \"Your SMS Text...\"\n"
		+ separators
		+ "[--sender Sender]    (11 alphanumeric or 16 numeric chars)\n"
		+ separators
		+ "[--flash 1 | 0]\n"

		+ separators
		+ "[--delay 1 | 0]\n"
		+ separators
		+ "[--udh 050003CC0201]\n"
		+ separators
		+ "[--ttl 120]	\n"
		+ separators
		+ "[--label YourLabel]\n"
		+ separators
		+ "[--performance_tracking 1 | 0]\n"
		+ separators
		+ "[--foreign_id YourId12345]\n"

		+ separators
		+ "[--logfile logfile.txt]\n"
		+ separators
		+ "[--config config.ini]\n"
		+ separators
		+ "\nAll Parameters can also be specified in config.ini file.\n"
		+ "\n";

	fprintf(stderr, usage_info.c_str(), argv[0]);
}

bool is_number(string s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

int myStoi(string s) {
	int number;
	stringstream ss(s);
	ss >> number;
	return number;
}

time_t timeStringToTimeT(string mytime) {
	struct tm vartime;
	string hr_time, min_time, sec_time;
	stringstream parsedtime;
	stringstream parsedtimestream(mytime);

	getline(parsedtimestream, hr_time, ':' );
	getline(parsedtimestream, min_time, ':' );

	if(!is_number(hr_time) || !is_number(min_time))
		return (time_t)-1;

	time_t tmp_time = time(0);
	tm *info = localtime(&tmp_time);
	info->tm_hour = myStoi(hr_time);
	info->tm_min = myStoi(min_time);
	mktime(info);

	if (mktime(info) != (time_t)-1) {
		// info is now safe to use
		time_t converted = mktime(info);
		return converted;
	}
	else {
		return (time_t)-1;
	}
}

bool checkNowBetween(string start, string end) {
	time_t now = time(0);
	time_t time_start = timeStringToTimeT(start);
	time_t time_end = timeStringToTimeT(end);

	//cout << time_start << endl << now << endl << time_end << endl;

	if(time_start < now && time_end > now && time_start > 1 && time_end > 1)
		return true;

	return false;
}

void printversion(string version) {
	cout << "seven-cli Version: " << version << endl;
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const string currentDateTime() {
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
	// for more information about date/time format
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	return buf;
}
void log(string logfile, string logtext) {
	if(logfile == "")
		return;

	ofstream myfile;
	myfile.open (logfile.c_str(), std::ios::app);
	myfile << currentDateTime() << " " << logtext << endl << "===============" << endl;
	myfile.close();
}

bool is_file_exist(const char *fileName) {
    std::ifstream infile(fileName);
    return infile.good();
}

int main(int argc, char* argv[])
{
	string version = "1.8";
	string response, apikey, logfile, config = "config.ini";
	string empfaenger, smstext, absender = "", flash = "0", debug = "0";
	string delay = "0", udh = "", ttl = "2880", label = "", performance_tracking = "0", foreign_id = "";
	string probealarm_start, probealarm_ende;

	int c;
	ostringstream oss;

	// GET PARAMETERS
	const char * short_opt = "hptxmfeuzarolcv:?";
	struct option	long_opt[] =
	{
		{"help", no_argument, NULL, 'h'},
		{"apikey", required_argument, NULL, 'p'},
		{"to", required_argument, NULL, 't'},
		{"text", required_argument, NULL, 'x'},
		{"sender", required_argument, NULL, 'm'},
		{"flash", required_argument, NULL, 'f'},

		{"delay", required_argument, NULL, 'e'},
		{"udh", required_argument, NULL, 'u'},
		{"ttl", required_argument, NULL, 'z'},
		{"label", required_argument, NULL, 'a'},
		{"performance_tracking", required_argument, NULL, 'r'},
		{"foreign_id", required_argument, NULL, 'o'},
   
		{"logfile", required_argument, NULL, 'l'},
		{"config", required_argument, NULL, 'c'},
		{"version", no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0	}
	};
	while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
	{
		switch (c)
		{
			case 'c':
				config = optarg;
				break;
		}
	}
	optind = 1;

	// GET INI OPTIONS
	minIni ini(config);

	apikey = ini.gets("Benutzerdaten", "apikey");

	logfile = ini.gets("Benutzerdaten", "logfile");

	empfaenger = ini.gets("SMS", "empfaenger");
	smstext = ini.gets("SMS", "smstext");
	absender = ini.gets("SMS", "absender");
	flash = ini.gets("SMS", "flash");
	
	delay = ini.gets("SMS", "delay");
	udh = ini.gets("SMS", "udh");
	ttl = ini.gets("SMS", "ttl");
	label = ini.gets("SMS", "label");
	performance_tracking = ini.gets("SMS", "performance_tracking");
	foreign_id = ini.gets("SMS", "foreign_id");

	probealarm_start = ini.gets("Probealarm", "start");
	probealarm_ende = ini.gets("Probealarm", "ende");

	if(flash == "" || (flash != "1" && flash != "0"))
		flash = "0";
	if(debug == "" || (debug != "1" && debug != "0"))
		debug = "0";

	while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1)
	{
		switch (c)
		{
			case -1: /* no more arguments */
			case 0:  /* long options toggles */
				break;

			case 'p':
				apikey = optarg;
				break;
			case 't':
        if(is_file_exist(optarg)) {
          std::ifstream t(optarg);
          std::stringstream buffer;
          buffer << t.rdbuf();

          empfaenger = buffer.str();
        }
        else {
          empfaenger = optarg;
        }

				break;
			case 'x':
				smstext = optarg;
				break;
			case 'm':
				absender = optarg;
				break;
			case 'f':
				flash = optarg;
				break;
			
			case 'e':
				delay = optarg;
				break;

			case 'u':
				udh = optarg;
				break;

			case 'z':
				ttl = optarg;
				break;

			case 'a':
				label = optarg;
				break;

			case 'r':
				performance_tracking = optarg;
				break;

			case 'o':
				foreign_id = optarg;
				break;

			case 'l':
				logfile = optarg;
				break;
			case 'c':
				config = optarg;
				break;

			case ':':
			case '?':
			case 'h':
				log(logfile, "seven-cli called with insuffient params");
				usage(argv);
				return(-2);

			case 'v':
				printversion(version);
				return(0);
		}
	}

	if(apikey == "" || empfaenger == "" || smstext == "" ||
		(flash != "0" && flash != "1")) {
		log(logfile, "seven-cli called with insuffient arguments");
		usage(argv);
		return(-2);
	}

	// Check Probealarm
	bool isProbe = false;
	if(probealarm_start != "" && probealarm_ende != "") {
		isProbe = checkNowBetween(probealarm_start, probealarm_ende);
		if(isProbe) {
			log(logfile, "Abbruch: Probealarm");
			return(-3);
		}
	}
	// Check Probealarm END

	curl_global_init(CURL_GLOBAL_ALL);

	string url = "https://gateway.seven.io/api/sms";
	string data = "p=" + url_encode(apikey)
		+ "&to=" + url_encode(empfaenger)
		+ "&text=" + url_encode(smstext)
		+ "&from=" + url_encode(absender)
		
		+ "&delay=" + url_encode(delay)
		+ "&udh=" + url_encode(udh)
		+ "&ttl=" + url_encode(ttl)
		+ "&label=" + url_encode(label)
		+ "&performance_tracking=" + url_encode(performance_tracking)
		+ "&foreign_id=" + url_encode(foreign_id)

		+ "&details=1&error_details=1"
		+ "&debug=" + url_encode(debug)
		+ "&sendwith=batchsend77_" + url_encode(version);

	if(CURLE_OK == curl_read(url, data, oss))
	{
		response = oss.str();
	}

	string logtext = "TO: " + empfaenger;
		logtext += ", TEXT: " + smstext;
		logtext += ", FROM: " + absender;
		logtext += "\nRESPONSE: " + response;
	log(logfile, logtext);

	cout << "Response: " << response << endl;

	curl_global_cleanup();

	// errorlevel
	string number_response;
	stringstream foostream(response);
	getline(foostream, number_response);

	if(number_response == "100")
		return(0);
	else
		return(-1);
}
