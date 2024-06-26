#ifndef USERS_HPP
#define USERS_HPP

namespace Users
{
  void insert_user(std::string email_address, std::string password, std::string user_name, std::string file_location);
  bool valid_user(std::string email_address, std::string file_location);
  bool valid_password(std::string email_address, std::string password, std::string file_location);
  bool email_alredy_taken(std::string email_addres, std::string file_location);
  void insert_cookie (std::string email_address, std::string cookie, std::string file_location);
  bool check_cookie(std::string cookie, std::string file_location);
  bool remove_cookie(std::string cookie_value, std::string file_location); 
};



#endif