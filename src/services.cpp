#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include "user.h" 
#include "generator.h"
#include "services.h"
#include "logs.h"
#include <fstream>

void Services::config_routes(Pistache::Rest::Router& chat_router)
{
    Logs::write_log_data("              function: config_routes");

    Pistache::Rest::Routes::Get(chat_router, "/", Pistache::Rest::Routes::bind(&Services::get_login_site));
    Pistache::Rest::Routes::Post(chat_router, "/login", Pistache::Rest::Routes::bind(&Services::login_handler));
    Pistache::Rest::Routes::Get(chat_router, "/chatsite", Pistache::Rest::Routes::bind(&Services::get_chat_site));
    Pistache::Rest::Routes::Post(chat_router, "/registration", Pistache::Rest::Routes::bind(&Services::registration_handler));
    Pistache::Rest::Routes::Post(chat_router, "/reg/name", Pistache::Rest::Routes::bind(&Services::reg_name_validation));
    Pistache::Rest::Routes::Post(chat_router, "/reg/email", Pistache::Rest::Routes::bind(&Services::reg_email_validation));
    Pistache::Rest::Routes::Post(chat_router, "/reg/password", Pistache::Rest::Routes::bind(&Services::reg_password_validation));
    Pistache::Rest::Routes::Post(chat_router, "/reg/comfirm_password", Pistache::Rest::Routes::bind(&Services::reg_confirm_validation));
    Pistache::Rest::Routes::Post(chat_router, "/log_out", Pistache::Rest::Routes::bind(&Services::log_out_handler));
}

void Services::is_port_used(int port_num)
{
   int sockfd = socket(AF_INET, SOCK_STREAM, 0);
   struct sockaddr_in serv_addr;
   bzero(&serv_addr, sizeof (serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons (port_num);

   if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) >= 0) 
   {
        close(sockfd);
        Logs::write_log_data("              function: is_port_used --> Socket connetcted sucesfully, port is free!");
   } 
   else 
   {
        close (sockfd);
        Logs::write_log_data("              function: is_port_used --> Socket cant connect, the port is used.");
   }
}

std::string Services::url_decode(std::string body_str) 
{
    Logs::write_log_data("              function: url_decode");

    std::regex reg_obj_space("%20");
    body_str = std::regex_replace(body_str, reg_obj_space, " ");

    std::regex reg_obj_at("%40");
    body_str = std::regex_replace(body_str, reg_obj_at, "@");

    std::regex reg_obj_and("%26");
    body_str = std::regex_replace(body_str, reg_obj_and, "&");

    std::regex reg_obj_dollar("%27");
    body_str = std::regex_replace(body_str, reg_obj_dollar, "$");

    std::regex reg_obj_exclam("%21");
    body_str = std::regex_replace(body_str, reg_obj_exclam, "!");

    std::regex reg_obj_plus("%2B");
    body_str = std::regex_replace(body_str, reg_obj_plus, "+");

    std::regex reg_obj_comma("%2C");
    body_str = std::regex_replace(body_str, reg_obj_comma, ",");

    std::regex reg_obj_per("%3A");
    body_str = std::regex_replace(body_str, reg_obj_per, "/");

    std::regex reg_obj_t_points("%3B");
    body_str = std::regex_replace(body_str, reg_obj_t_points, ":");

    std::regex reg_obj_e_comma("%3D");
    body_str = std::regex_replace(body_str, reg_obj_e_comma, ";");

    std::regex reg_obj_eq("%24");
    body_str = std::regex_replace(body_str, reg_obj_eq, "=");

    std::regex reg_obj_question_m("%3F");
    body_str = std::regex_replace(body_str, reg_obj_question_m, "?");

    std::regex reg_obj_apos("%60");
    body_str = std::regex_replace(body_str, reg_obj_apos, "'");

    Logs::write_log_data("              function: url_decode end, url data: " + body_str);
    return body_str;
}

bool Services::is_valid_session(const Request &request)
{ 
    Logs::write_log_data("              function: is_valid_session");

    try 
    {
        auto cookies = request.cookies();

        auto cookie = cookies.get("login_cookie");
        std::string cookie_token = cookie.value;
        Logs::write_log_data("              function: is_valid_session, cookie token: " + cookie_token);

        if (Users::check_cookie(cookie.value, "../../db/Users.db3"))
        {
            Logs::write_log_data("              function: is_valid_session, return: true");

            return true;
        }
    } 
    catch (std::exception &e) 
    {
        Logs::write_log_data("              function: is_valid_session");
        return false;
    }

    return false;
}

Pistache::Http::Cookie Services::login_cookie_generator(std::string email)
{
    Logs::write_log_data("              function: login_cookie_generator");

    std::string cookie_value = Generators::random_number_generator(5);
 
    Pistache::Http::Cookie cookie("login_cookie", cookie_value);

    Users::insert_cookie(email, cookie_value, "../../db/Users.db3");
    
    cookie.maxAge.emplace(3600);

    return cookie;
}

void Services::get_login_site(const Request &request, Response response)
{
    try 
    {
        Logs::write_log_data(" GET request, function: get_login_site");

        if(is_valid_session(request))
        {
            response.headers().add<Pistache::Http::Header::Location>("/chatsite");
            response.send(Pistache::Http::Code::See_Other);
        }
 
        const std::string uri = request.resource();

        std::string htmlContent;
        std::ifstream htmlFile("../../frontend/loginpage/index.html");
        std::getline(htmlFile, htmlContent, '\0');

        std::string cssContent;
        std::ifstream cssFile("../../frontend/loginpage/style.css");
        std::getline(cssFile, cssContent, '\0');

        htmlContent += "\n<style>" + cssContent + "</style>";

        response.headers().add<Pistache::Http::Header::ContentType>(MIME(Text, Html));
        response.send(Http::Code::Ok, htmlContent);
    } 
    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: get_login_site, ", e);
    }
}

void Services::login_handler(const Request &request, Response response)
{
    try 
    {
        std::string body_data = url_decode(request.body());
        int and_simbol = body_data.find("&");
   
        std::string email_addres = body_data.substr(9, and_simbol - 9); 
        std::string password = body_data.substr(and_simbol + 10);

        Logs::write_log_data("POST request, function: login_handler", email_addres);

        if (Users::valid_user(email_addres, "../../db/Users.db3") == false)
        {
        
            std::string login_email_validation = R"(
            <div hx-swap="outerHTML">
				<button hx-post="/login"
					hx-target="closest div"
					hx-include="closest form">Sign In
			    </button>
                <div class='error-message'>Wrong email address.</div>
		    </div>
            )";

            response.send(Http::Code::Ok, login_email_validation);

        } 
        else if (!Users::valid_password(email_addres, password, "../../db/Users.db3"))
        {
            std::string login_poassword_validation = R"(
            <div hx-swap="outerHTML">
				<button hx-post="/login"
					hx-target="closest div"
					hx-include="closest form" ">Sign In
			    </button>
                <div class='error-message'>Wrong password.</div>
		    </div>
            )";

            response.send(Http::Code::Ok, login_poassword_validation);
        }
        else
        {
            response.headers().add<Pistache::Http::Header::Location>("/chatsite");
            response.cookies().add(login_cookie_generator(email_addres));
            response.headers().addRaw(Pistache::Http::Header::Raw{"HX-Redirect", ""});
            response.send(Pistache::Http::Code::See_Other);
        }
    } 
    catch (const std::exception &e) 
    {
        std::string login_validation_error = R"(
        <div hx-swap="outerHTML">
			<button hx-post="/login"
				hx-target="closest div"
				hx-include="closest form">Sign In
			</button>
            <div class='error-message'>Server error.</div>
		</div>
        )";

        response.send(Http::Code::Ok, login_validation_error);

        Logs::write_log_data_exception("              function: login_handler, ", e);
    }

}

void Services::get_chat_site(const Request &request, Response response)
{
    try {
        Logs::write_log_data(" GET request, function: get_chat_site");

        auto cookies = request.cookies();

        if(!cookies.has("login_cookie")) // && !Users::check_cookie(cookies.get("login_cookie").value, "../../db/Users.db3")
        {

            response.headers().add<Pistache::Http::Header::Location>("/");
            response.send(Pistache::Http::Code::See_Other);
        }
        if(!Users::check_cookie(cookies.get("login_cookie").value, "../../db/Users.db3")) // 
        {

            response.headers().add<Pistache::Http::Header::Location>("/");
            response.send(Pistache::Http::Code::See_Other);
        }
    
    

        std::string htmlContent;
        std::ifstream htmlFile("../../frontend/chatsite/index.html");
        std::getline(htmlFile, htmlContent, '\0');

        std::string cssContent;
        std::ifstream cssFile("../../frontend/chatsite/style.css");
        std::getline(cssFile, cssContent, '\0');

        htmlContent += "\n<style>" + cssContent + "</style>";

        response.headers().add<Pistache::Http::Header::ContentType>(MIME(Text, Html));

        response.send(Http::Code::Ok, htmlContent);
    } 
    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: get_chat_site, ", e);
    }
}

void Services::reg_email_validation(const Request &request, Response response)
{
    try {
        std::string body_data = url_decode(request.body());

        std::string user_name = body_data.substr(5, body_data.find ("&") - 5);
        std::string first_arese = body_data.erase(0, body_data.find ("&") + 1);
        std::string email_addres = first_arese.substr(6, body_data.find ("&") - 6);

        Logs::write_log_data("POST request, function: reg_email_validation", email_addres);

        std::regex pattern("^\\w+.\\w+@\\w+.com$");
        bool valid_email = std::regex_match (email_addres, pattern);

        if (Users::email_alredy_taken(email_addres, "../../db/Users.db3") == true)
        {
            std::string email_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
				<input type="email" name="email" value=")" + email_addres + R"(" placeholder="Email" hx-post="/reg/email" hx-indicator="#ind"/>
                <div class='error-message' >This email addres is alredy taken.</div>
            </div>
            )";

            response.send(Http::Code::Ok, email_part);
            return;
        }
        if (valid_email == true)
        {
            std::string email_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
				<input type="email" name="email" value=")" + email_addres + R"(" placeholder="Email" hx-post="/reg/email" hx-indicator="#ind"/>
		    </div>
            )";

            response.send(Http::Code::Ok, email_part);
            return;
        }
        if (valid_email == false)
        {
            std::string email_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
				<input type="email" name="email" value=")" + email_addres + R"(" placeholder="Email" hx-post="/reg/email" hx-indicator="#ind"/>
                <div class='error-message' >Please enter a valid email address.</div>
            </div>
            )";

            response.send(Http::Code::Ok, email_part);
            return;
        }
        } 
        catch (const std::exception &e) 
        {
            Logs::write_log_data_exception("              function: reg_email_validation, ", e);
        }

}

void Services::reg_name_validation(const Request &request, Response response)
{
    try 
    {
        std::string body_data = url_decode(request.body());
        std::string user_name = body_data.substr(5, body_data.find("&") - 5);

        Logs::write_log_data("POST request, function: reg_name_validation", user_name);

        std::regex pattern("^[A-Z][a-z]+\\s[A-Z][a-z]+$");
        bool name_validating = std::regex_match(user_name, pattern);

        if (name_validating == true)
        {
            std::string name_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
                <input type="text" name="name" value=")" + user_name + R"(" placeholder="Name" hx-post="/reg/name" hx-indicator="#ind"/>
            </div>
            )";

            response.send(Http::Code::Ok, name_part);
        }
        else
        {
            std::string name_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
                <input type="text" name="name" value=")" + user_name + R"(" placeholder="Name" hx-post="/reg/name" hx-indicator="#ind"/>
                <div class='error-message' >Your name should contain 2 words start with apper letters.</div>
            </div>
            )";

        response.send(Http::Code::Ok, name_part);
        }
    } 
    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: reg_name_validation, ", e);
    }
    }

void Services::reg_password_validation(const Request &request, Response response)
{
    try 
    {
        Logs::write_log_data("POST request, function: reg_password_validation");

        std::string body_data = url_decode(request.body());

        std::string user_name = body_data.substr(5, body_data.find ("&") - 5);
        std::string first_arese = body_data.erase(0, body_data.find ("&") + 1);
        std::string email_addres = first_arese.substr(6, body_data.find ("&") - 6);
        std::string second_erase = first_arese.erase(0, first_arese.find ("&") + 1);
        std::string password = second_erase.substr(9, second_erase.find ("&") - 9);

        std::regex pattern("^(?=.*[A-Za-z])(?=.*\\d)[A-Za-z\\d]{8,}$");
        bool valid_password = std::regex_match(password, pattern);

        if (valid_password == true)
        {
            std::string password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <input type="password" name="password" value=")" + password + R"(" placeholder="Password" hx-post="/reg/passweord" hx-indicator="#ind"/>
		    </div>
            )";

            response.send (Http::Code::Ok, password_part);
        }
        else
        {
            std::string password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <input type="password" name="password" value=")" + password + R"(" placeholder="Password" hx-post="/reg/passweord" hx-indicator="#ind"/>
                <div class='error-message' >Minimum eight characters, at least one uppercase letter, one lowercase letter and one number.</div>
            </div>
            )";

            response.send(Http::Code::Ok, password_part);
        }
    } 
    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: reg_password_validation, ", e);
    }
}

void Services::reg_confirm_validation(const Request &request, Response response)
{
   try {
        std::string body_data = url_decode (request.body());

        std::string user_name = body_data.substr(5, body_data.find ("&") - 5);
        std::string first_erase = body_data.erase( 0, body_data.find ("&") + 1);
        std::string email_address = first_erase.substr(6, body_data.find ("&") - 6);
        std::string second_erase = first_erase.erase(0, first_erase.find ("&") + 1);
        std::string password = second_erase.substr(9, second_erase.find ("&") - 9);
        std::string third_erase = first_erase.erase( 0, first_erase.find ("&") + 1);
        std::string confirm_password = third_erase.substr(third_erase.find ("=") + 1);
        
        Logs::write_log_data("POST request, function: reg_confirm_validation", email_address);

        if (password == confirm_password)
        {
            std::string confimr_password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <input type="password" name="repeta_password" value=")" + confirm_password + R"(" placeholder="Repeta password" hx-post="/reg/comfirm_passweord" hx-indicator="#ind"/>
		    </div>
            )";

            response.send(Http::Code::Ok, confimr_password_part);
        }
        else
        {
            std::string confimr_password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <input type="password" name="repeta_password" value=")" + confirm_password + R"(" placeholder="Repeta password" hx-post="/reg/comfirm_passweord" hx-indicator="#ind"/>
                <div class='error-message' >Your two password should be the same.</div>       
            </div>
            )";

            response.send(Http::Code::Ok, confimr_password_part);
        }
    } 

    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: reg_confirm_validation, ", e);
    }
}

void Services::registration_handler(const Request &request, Response response)
{
    try {
        std::string body_data = url_decode(request.body());

        std::string user_name = body_data.substr(5, body_data.find("&") - 5);
        std::string first_arese = body_data.erase(0, body_data.find("&") + 1);
        std::string email_addres = first_arese.substr(6, body_data.find("&") - 6);
        std::string second_erase = first_arese.erase(0, first_arese.find("&") + 1);
        std::string password = second_erase.substr(9, second_erase.find("&") - 9);
        std::string third_erase = first_arese.erase(0, first_arese.find("&") + 1);
        std::string confirm_password = third_erase.substr(third_erase.find("=") + 1);

        Logs::write_log_data("POST request, function: registration_handler", email_addres);

        std::regex pattern_email("^\\w+.\\w+@\\w+.com$");
        bool valid_email = std::regex_match(email_addres, pattern_email);

        if (valid_email == false)
        {
            std::string email_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <button type="submit" hx-post="/registration" hx-include="closest form" hx-trigger="click">Sign Up</button>
                <div class='error-message'>Please correct your data!</div>
            </div>
            )";

            response.send(Http::Code::Ok, email_part);
            return;
        }
    
        std::regex pattern_name("^[A-Z][a-z]+\\s[A-Z][a-z]+$");
        bool name_validating = std::regex_match(user_name, pattern_name);

        if (name_validating == false)
        {
            std::string name_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <button type="submit" hx-post="/registration" hx-include="closest form" hx-trigger="click">Sign Up</button>
                <div class='error-message'>Please correct your data!</div>
            </div>
            )";

        response.send(Http::Code::Ok, name_part);
        return;
        }

        std::regex pattern_password("^(?=.*[A-Za-z])(?=.*\\d)[A-Za-z\\d]{8,}$");
        bool valid_password = std::regex_match(password, pattern_password);

        if (valid_password == false)
        {
            std::string password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <button type="submit" hx-post="/registration" hx-include="closest form" hx-trigger="click">Sign Up</button>
                <div class='error-message'>Please correct your data!</div>
            </div>
            )";

            response.send(Http::Code::Ok, password_part);
            return;
        }

        if (password != confirm_password)
        {
        std::string confimr_password_part = R"(
            <div hx-target="this" hx-swap="outerHTML">
			    <button type="submit" hx-post="/registration" hx-include="closest form" hx-trigger="click">Sign Up</button>
                <div class='error-message'>Please correct your data!</div>
            </div>
            )";

            response.send(Http::Code::Ok, confimr_password_part);
            return;
        }


        Users::insert_user(email_addres, password, user_name, "../../db/Users.db3");

        response.headers().add<Pistache::Http::Header::Location>("/chatsite");
        response.send(Pistache::Http::Code::See_Other);
    } 
    catch (const std::exception &e) 
    {
        Logs::write_log_data_exception("              function: registration_handler, ", e);
    }
}

void Services::log_out_handler(const Pistache::Rest::Request &request, Pistache::Http::ResponseWriter response)
{
    Logs::write_log_data("POST request, function: log_out_handler");

    try 
    {
        auto cookies = request.cookies ();
        auto cookie_token = cookies.get("login_cookie");
        std::string login_cookie = cookie_token.value;

        Users::remove_cookie(login_cookie, "../../db/Users.db3"); 
    
        response.headers ().add<Pistache::Http::Header::Location> ("/");
        response.headers ().addRaw(Pistache::Http::Header::Raw{"HX-Redirect", ""});
        response.send (Pistache::Http::Code::See_Other);
    } 
    catch(const std::exception &e) 
    {

        Logs::write_log_data_exception("              function: log_out_handler, ", e);  
    }
}

