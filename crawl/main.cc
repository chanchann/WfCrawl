
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <regex>
#include <iostream>
#include "workflow/HttpMessage.h"
#include "workflow/HttpUtil.h"
#include "workflow/WFTaskFactory.h"
#include "workflow/WFFacilities.h"

#define REDIRECT_MAX    5
#define RETRY_MAX       2

static WFFacilities::WaitGroup wait_group(1);

void addPrefix(std::string& url) {
	if (strncasecmp(url.c_str(), "http://", 7) != 0 
                && strncasecmp(url.c_str(), "https://", 8) != 0) {
		url = "http://" + url;
	}
}

void parseContent(const std::string& content) {
	// <a href="#形象"><span class="tocnumber">2</span> <span class="toctext">形象</span></a>
	// <a href="([^"]+)"><span class="tocnumber">[0-9]*</span> <span class="toctext">([^"]+)</span></a>
	std::string str = "<a href=\"([^\"]+)\"><span class=\"tocnumber\">[0-9]*</span> <span class=\"toctext\">([^\"]+)</span></a>";
	std::cout << "str : " << str << std::endl;
    std::regex e(str);
    std::smatch m; 
    bool found = std::regex_search(content, m, e);
    std::cout << "result size = " << m.size() << std::endl;
    for(size_t i = 0; i < m.size(); i++){
        std::cout << "m[0]: " << m[0] << " m[1]: " << m[1] 
                << " m[2]: " << m[2] << std::endl; 
    }
}

void sig_handler(int signo)
{
	wait_group.done();
}

void wget_callback(WFHttpTask *task)
{
	protocol::HttpRequest *req = task->get_req();
	protocol::HttpResponse *resp = task->get_resp();
	int state = task->get_state();
	int error = task->get_error();

	switch (state)  {
        case WFT_STATE_SYS_ERROR:
            fprintf(stderr, "system error: %s\n", strerror(error));
            break;
        case WFT_STATE_DNS_ERROR:
            fprintf(stderr, "DNS error: %s\n", gai_strerror(error));
            break;
        case WFT_STATE_SSL_ERROR:
            fprintf(stderr, "SSL error: %d\n", error);
            break;
        case WFT_STATE_TASK_ERROR:
            fprintf(stderr, "Task error: %d\n", error);
            break;
        case WFT_STATE_SUCCESS:
            break;
	}

	if (state != WFT_STATE_SUCCESS) {
		fprintf(stderr, "Failed. Press Ctrl-C to exit.\n");
		return;
	}

	std::string name;
	std::string value;

	/* Print request. */
	fprintf(stderr, "%s %s %s\r\n", req->get_method(),
									req->get_http_version(),
									req->get_request_uri());

	protocol::HttpHeaderCursor req_cursor(req);

	while (req_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

	/* Print response header. */
	fprintf(stderr, "%s %s %s\r\n", resp->get_http_version(),
									resp->get_status_code(),
									resp->get_reason_phrase());

	protocol::HttpHeaderCursor resp_cursor(resp);

	while (resp_cursor.next(name, value))
		fprintf(stderr, "%s: %s\r\n", name.c_str(), value.c_str());
	fprintf(stderr, "\r\n");

	/* Print response body. */
	const void *body;
	size_t body_len;
    
	resp->get_parsed_body(&body, &body_len);
    std::string bodystr(static_cast<const char*>(body), body_len);
	parseContent(bodystr);
	// fwrite(body, 1, body_len, stdout);
	// fflush(stdout);

	fprintf(stderr, "\nSuccess. Press Ctrl-C to exit.\n");
}


int main(int argc, char *argv[]) {
	WFHttpTask *task;
    std::string url = "https://zh.moegirl.org.cn/%E5%98%89%E7%84%B6";

    addPrefix(url);

	signal(SIGINT, sig_handler);

	task = WFTaskFactory::create_http_task(url, 
                                            REDIRECT_MAX,
                                            RETRY_MAX,
										    wget_callback);
	// todo : 通过读入配置文件
	std::string cookie = R"(_vwo_uuid_v2=DB3E2F8C24DEF8098CF5ED5DD9C0FC415|5f1ba9cc238a6362a96de38f9917b22f; gr_user_id=a64b6309-dbfa-4a3f-af30-5f11a4e77a9d; douban-fav-remind=1; 
			__gads=ID=fe65bfbdb8cbb05b-222346b028c50025:T=1607839629:RT=1607839629:S=ALNI_MaF3OQn8FVJ8uYQ0KLLS9SnaoBdgg; 
			ll="118318"; bid=5Kk0QMfgT1Y; viewed="6709783_26912767_27036085"; __utmz=30149280.1624867847.6.5.utmcsr=cn.bing.com|utmccn=(referral)|utmcmd=referral|utmcct=/; 
			dbcl2="183972483:ECvJipOSpS4"; ck=-vNN; ap_v=0,6.0; push_noty_num=0; push_doumail_num=0; __utmc=30149280; __utmv=30149280.18397; 
			_pk_ref.100001.8cb4=%5B%22%22%2C%22%22%2C1629175111%2C%22https%3A%2F%2Fwww.google.com%2F%22%5D; 
			_pk_ses.100001.8cb4=*; _pk_id.100001.8cb4=ae67bfaac25be04d.1624027232.3.1629176043.1629169490.; __utma=30149280.389361898.1608820758.1629169501.1629176055.8; 
			__utmt=1; __utmb=30149280.2.10.1629176055
	)";
	protocol::HttpRequest *req = task->get_req();
	req->add_header_pair("Accept", "*/*");
	req->add_header_pair("User-Agent", "Wget/1.14 (linux-gnu)");
	req->add_header_pair("Connection", "close");
	req->add_header_pair("Cookie", cookie);
	task->start();

	wait_group.wait();
	return 0;
}
