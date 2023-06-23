#ifndef LEX_IDENT_INCLUDED
#define LEX_IDENT_INCLUDED
/*
   Copyright (c) 2023, MariaDB

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; version 2 of
   the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1335  USA
*/

#include "char_buffer.h"


class Lex_ident_fs: public LEX_CSTRING
{
public:
  Lex_ident_fs()
   :LEX_CSTRING({0,0})
  { }
  Lex_ident_fs(const char *str, size_t length)
   :LEX_CSTRING({str, length})
  { }
  explicit Lex_ident_fs(const LEX_CSTRING &str)
   :LEX_CSTRING(str)
  { }
  static bool check_body(const char *name, size_t length,
                         bool disallow_path_chars);
  bool check_db_name() const;
  bool check_db_name_with_error() const;
  bool check_db_name_quick() const
  {
    return !length || length > NAME_LEN || str[length-1] == ' ';
  }
};



template<size_t buff_sz>
class Casedn_ident_buffer: public CharBuffer<buff_sz>
{
  constexpr static CHARSET_INFO *charset_info()
  {
    return &my_charset_utf8mb3_general_ci;
  }
public:
  Casedn_ident_buffer()
   :CharBuffer<buff_sz>()
  { }
  Casedn_ident_buffer(const LEX_CSTRING &str)
   :CharBuffer<buff_sz>()
  {
    copy_casedn(str);
  }
  Casedn_ident_buffer(const LEX_CSTRING &str, bool casedn)
   :CharBuffer<buff_sz>()
  {
    copy_casedn(str, casedn);
  }
  Casedn_ident_buffer<buff_sz> & copy_casedn(const LEX_CSTRING &str)
  {
    CharBuffer<buff_sz>::copy_casedn(charset_info(), str);
    return *this;
  }
  Casedn_ident_buffer<buff_sz> & copy_casedn(const LEX_CSTRING &str, bool casedn)
  {
    CharBuffer<buff_sz>::copy_casedn(charset_info(), str, casedn);
    return *this;
  }
  bool check_db_name() const
  {
    return Lex_ident_fs(CharBuffer<buff_sz>::to_lex_cstring()).
             check_db_name();
  }
  bool check_db_name_with_error() const
  {
    return Lex_ident_fs(CharBuffer<buff_sz>::to_lex_cstring()).
             check_db_name_with_error();
  }
};


#endif // LEX_IDENT_INCLUDED
