#include <iostream>
#include <cstdio>
#include <sstream>
#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <json/json.h>
#include <pthread.h>
#include <unistd.h>
#include "base/http.h"
#include "speech.h"


#define ASR_PATH "./temp_file/asr.wav"
#define TTS_PATH "./temp_file/tts.wav"
#define CMD_ETC "./command.etc"
#define KEY_ETC "./key.etc"
class Robort
{
  private:

    std::string api_key = "06bef4f34ae842d38d1255fe6ca66298";
    std::string user_id = "110";
    std::string url = "openapi.tuling123.com/openapi/api/v2";
    aip::HttpClient client;

    std::string MessageToJson(std::string message)
    {

		//if(message.empty())
		//{
		//	message = "人器机灵图";
		//}
		//std::cout << message << std::endl;
      Json::Value root;
      Json::Value item1;
      Json::Value item2;
      Json::StreamWriterBuilder wb;
      std::ostringstream os;

      root["reqType"] = "0";
      item1["text"] = message;
      item2["inputText"] = item1;
      root["perception"] = item2;

      item1.clear();
      item1["apiKey"] = api_key;
      item1["userId"] = user_id;
      root["userInfo"] = item1;


      std::unique_ptr<Json::StreamWriter> jw(wb.newStreamWriter());

      jw->write(root, &os);
      std::string str = os.str();
      return str;

    }

    std::string Request(std::string str)
    {
      std::string response;

      int code = client.post(url, nullptr, str, nullptr, &response);
      if(code != CURLcode::CURLE_OK)
      {
        std::cerr<< "Request error" <<  std::endl;
        return "";
      }
      return response;

    }
	bool IsCodeLegal(int code)
	{

	}


    std::string JsonToEchoMessage(std::string str)
    {
      JSONCPP_STRING errs;
      Json::Value root;
      Json::CharReaderBuilder rb;
      std::unique_ptr<Json::CharReader> const jr(rb.newCharReader());
      bool res = jr->parse(str.data(), str.data()+str.size(), &root, &errs);
      if(!res || !errs.empty())
      {
       // std::cerr << "json to message error" << std::endl;
        return "";

      }
      return root["results"][0]["values"]["text"].asString();
    }
  public:
    std::string Talk(std::string str)
    {
		//std::cout << str << std::endl;
	  
      std::string json = this->MessageToJson(str);
      std::string echojson = this->Request(json);

	
      std::string echomessage = this->JsonToEchoMessage(echojson);
	
      return echomessage;
    }
    Robort()
    {
      
    }
    ~Robort()
    {

    }

};

class Util
{
public:
	static bool Exec(std::string cmd, bool Is_print)
	{
		if(!Is_print)
		{
			cmd +=  " >/dev/null 2>&1";
		}
		FILE *fp = popen(cmd.c_str(), "r");
		if(nullptr == fp)
		{
			std::cerr << "Exec error" <<  std::endl;
			return false;
		}
		char s;
		while(fread(&s, 1, 1, fp) > 0)
		{
			std::cout << s;
		}
		pclose(fp);
		return true;
	}

	static void Load(std::string  path, std::unordered_map<std::string, std::string>& map)
	{
		char buf[256];
		std::ifstream in(path.c_str());
		if(!in.is_open())
		{
			std::cerr << "open file failed" << std::endl;
			exit(1);
		}
		std::string seq = ":";
		while(in.getline(buf, sizeof(buf)))
		{
			std::string str = buf;
			size_t index = str.find(seq);
			if(index == std::string::npos)
			{
				std::cerr << "seq not find." << std::endl;
				continue;
			}
			std::string msg = str.substr(0, index);
			std::string cmd = str.substr(index + seq.size());
			map[msg] = cmd;
		}
	}



};

//语音识别---语言合成
class SpeechRec{

	private:
		std::string api_id = "16868899";
		std::string api_key = "T2kMGGMnwbpXDCet49z1mhm8";
		std::string secret_key = "YppTsbRriyOPV8FPWgFs6Wh44rwHe2jP";

		aip::Speech* client = new aip::Speech(api_id, api_key, secret_key);

	public:
		
		bool ASR(std::string& message)
		{
			std::string file_content;
			aip::get_file_content(ASR_PATH, &file_content);
			std::map<std::string, std::string> options;
			options["dev_pid"] = "1536";
			Json::Value root = client->recognize(file_content, "wav", 16000, options);
			//std::cout << root.toStyledString() << std::endl;
			if(root["err_no"].asInt() != 0 )
			{
				message = "人器机灵图";
				return true;
			}
			message = root["result"][0].asString();
			return true;
		}

		bool TTS(std::string message)
		{
			std::ofstream ofile;
			std::string file_ret;
			std::map<std::string, std::string> options;
				options["per"] = "111";
				options["aue"] = "6";
			ofile.open(TTS_PATH, std::ios::binary);
			Json::Value result = client->text2audio(message, options,file_ret);
			if(!file_ret.empty())
			{
				ofile << file_ret;
			}
			else
			{

				std::cerr << "text to audio false : " <<result["err_msg"] << std::endl;
			}
			ofile.close();
		}


	SpeechRec()
	{

	}

	~SpeechRec()
	{

	}
};


class Jarvis
{
	private:
		SpeechRec sh;
		Robort rt;

		std::unordered_map<std::string, std::string> msg_cmd;
	private:

		bool Record()
		{
			std::cout << "Recording..." << std::endl;
			std::string cmd = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
			cmd += ASR_PATH;
			if(!Util::Exec(cmd, false))
			{
				return false;
			}
			return true;
		}

		bool Play()
		{
			std::string cmd = "cvlc --play-and-exit ";
			cmd += TTS_PATH;
			if(!Util::Exec(cmd, false))
			{
				std::cerr << " play error " << std::endl;
				return false;
			}
			return true;
		}

		bool Is_cmd(const std::string&recog_msg, std::string& cmd )
		{
			auto it  = msg_cmd.find(recog_msg);
			if(it == msg_cmd.end())
				return false;
			else
			{
				cmd = it->second;
				return true;
			}
		}

	public:
		void Run()
		{
			if(!Record())
			{
				return;
			 }
			std::cout << "Record" << std::endl;
			std::string request_str;
			if(!sh.ASR(request_str))
			{
				return;
			}

			std::string cmd;
			if (request_str != "人器机灵图")
				std::cout << "宗介#" << request_str << std::endl;

			if(Is_cmd(request_str, cmd))//是命令---
			{
				std::cout << "[Fake_Jarvis$]" << cmd << std::endl;
				if(!Util::Exec(request_str, true))
					return;
			}
			else//不是命令---
			{

				std::string echo_str = rt.Talk(request_str);
				sh.TTS(echo_str);
				std::cout << "波妞#"<< echo_str <<std::endl;
				Play();
				if(request_str == "去休息吧。")
					exit(0);
			}
		}
	Jarvis()
	{
		Util::Load(CMD_ETC, msg_cmd);
	}

	~Jarvis()
	{
	}

	
};
