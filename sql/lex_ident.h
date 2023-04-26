/*
   Copyright (c) 2018, 2023, MariaDB Corporation.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef LEX_IDENT_INCLUDED
#define LEX_IDENT_INCLUDED

#include "lex_string.h"

extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *table_alias_charset;

/*
  LEX_CSTRING with comparison semantics.
*/

// db and table names: case sensitive (or insensitive) in table_alias_charset
struct Compare_table_names
{
  CHARSET_INFO *charset_info() const
  {
    return table_alias_charset;
  }
};


// case insensitive identifiers
struct Compare_ident_ci
{
  CHARSET_INFO *charset_info() const
  {
    return &my_charset_utf8mb4_tolower_ci;
  }
};


/*
  Possible identifier values:
    1.  {ptr==NULL,length==0} is valid and means "NULL identifier".
    2a. {ptr<>NULL,length==0} means "empty identifier".
    2b. {ptr<>NULL,length>0}  means "not empty identifier.
  In case of 2a and 2b, ptr must be a '\0'-terninated string.

  Comparison operands passed to streq() are not required to be 0-terminated.

  Debug validation is done during comparison time:
  - inside methods of this class
  - inside st_charset_info::streq() in include/m_ctype.h
  The caller must make sure to maintain the object in the valid state,
  as well as provide valid LEX_CSTRING instances for comparion.

  For better code stability, the Lex_cstring base should eventually be
  encapsulated, so the object debug validation is done at constructor
  time rather than at comparison time.
*/

template <class Compare>
class Lex_ident : public Lex_cstring
{
protected:
  // Make sure the object is valid
  bool is_valid_ident() const
  {
    // NULL identifier, or 0-terminated identifier
    return (str == NULL && length == 0) || str[length] == 0;
  }
public:

  constexpr Lex_ident() = default;
  explicit constexpr Lex_ident(const LEX_CSTRING &str)
   :Lex_cstring(str)
  { }
  constexpr Lex_ident(const char *str, size_t len)
   :Lex_cstring(str, len)
  { }
  Lex_ident(const char *start, const char *end)
   :Lex_ident(start, end)
  { }
  Lex_ident(const Lex_cstring_strlen &str)
   :Lex_cstring(str)
  { }
  explicit operator bool () const
  {
    return str != NULL;
  }
  static CHARSET_INFO *charset_info()
  {
    return Compare().charset_info();
  }
  /*
    Compare two not necessarily 0-terminated LEX_CSTRING instances.
    Both sides can also be valid NULL identifiers.
  */
  static bool streq(const LEX_CSTRING &a, const LEX_CSTRING &b)
  {
    return Compare().charset_info()->streq(a, b);
  }
  /*
    Compare the object to a not necessarily 0-terminated LEX_CSTRING.
    Both "this" and rhs can also be NULL identifiers.
  */
  bool streq(const LEX_CSTRING &rhs) const
  {
    DBUG_ASSERT(is_valid_ident());
    return Compare().charset_info()->streq(*this, rhs);
  }
  /*
    Compare two objects.
    Both "this" and rhs can also be NULL identifiers.
  */
  bool streq(const Lex_ident &b) const
  {
    DBUG_ASSERT(is_valid_ident());
    DBUG_ASSERT(b.is_valid_ident());
    return Compare().charset_info()->streq(*this, b);
  }
};


/*
  An identifier whose case sensitivity depends on table_alias_charset, i.e. on
  - the datadir filesystem case sensitivity, and
  - the value of --lower-case-table-names
*/
class Lex_ident_lc: public Lex_ident<Compare_table_names>
{
public:
  using Lex_ident::Lex_ident;
};


class Lex_ident_db: public Lex_ident_lc
{
public:
  using Lex_ident_lc::Lex_ident_lc;
};


class Lex_ident_table: public Lex_ident_lc
{
public:
  using Lex_ident_lc::Lex_ident_lc;
};


class Lex_ident_trigger: public Lex_ident_lc
{
public:
  using Lex_ident_lc::Lex_ident_lc;
};


/*
  A case insensitive identifier.
*/
class Lex_ident_ci: public Lex_ident<Compare_ident_ci>
{
public:
  using Lex_ident::Lex_ident;
};


class Lex_ident_column: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_sys_var: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_user_var: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_sp_var: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_ps: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_i_s_table: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_window: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_func: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_partition: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_with_element: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_rpl_filter: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_master_info: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_host: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_locale: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_plugin: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_engine: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_server: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_savepoint: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_table_option_name: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_table_option_value: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


class Lex_ident_charset: public Lex_ident_ci
{
public:
  using Lex_ident_ci::Lex_ident_ci;
};


static inline constexpr
Lex_ident_db operator"" _Lex_ident_db(const char *str, size_t length)
{
  return Lex_ident_db(str, length);
}


static inline constexpr
Lex_ident_table operator"" _Lex_ident_table(const char *str, size_t length)
{
  return Lex_ident_table(str, length);
}


static inline constexpr
Lex_ident_column operator"" _Lex_ident_column(const char *str, size_t length)
{
  return Lex_ident_column(str, length);
}

static inline constexpr
Lex_ident_func operator"" _Lex_ident_func(const char *str, size_t length)
{
  return Lex_ident_func(str, length);
}

static inline constexpr
Lex_ident_engine operator"" _Lex_ident_engine(const char *str, size_t length)
{
  return Lex_ident_engine(str, length);
}


static inline constexpr
Lex_ident_sp_var operator"" _Lex_ident_sp_var(const char *str, size_t length)
{
  return Lex_ident_sp_var(str, length);
}


static inline constexpr
Lex_ident_locale operator"" _Lex_ident_locale(const char *str, size_t length)
{
  return Lex_ident_locale(str, length);
}


static inline constexpr
Lex_ident_charset operator"" _Lex_ident_charset(const char *str, size_t length)
{
  return Lex_ident_charset(str, length);
}


static inline constexpr
Lex_ident_plugin operator"" _Lex_ident_plugin(const char *str, size_t length)
{
  return Lex_ident_plugin(str, length);
}


#endif // LEX_IDENT_INCLUDED
