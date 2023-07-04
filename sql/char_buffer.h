#ifndef CHAR_BUFFER_INCLUDED
#define CHAR_BUFFER_INCLUDED
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

template<size_t buff_sz>
class CharBuffer
{
  char m_buff[buff_sz + 1];
  size_t m_length;
public:
  CharBuffer()
   :m_length(0)
  {
    m_buff[0]= '\0';
  }
  CharBuffer<buff_sz> & copy_bin(const LEX_CSTRING &str)
  {
    DBUG_ASSERT(str.length < sizeof(m_buff));
    m_length= MY_MIN(sizeof(m_buff) - 1, str.length);
    memcpy(m_buff, str.str, m_length);
    m_buff[m_length]= '\0';
    return *this;
  }
  CharBuffer<buff_sz> & copy_casedn(CHARSET_INFO *cs, const LEX_CSTRING &str)
  {
    m_length= cs->cset->casedn(cs, str.str, str.length, m_buff, buff_sz);
    DBUG_ASSERT(m_length < sizeof(m_buff));
    m_buff[m_length]= '\0';
    return *this;
  }
  CharBuffer<buff_sz> & copy_casedn(CHARSET_INFO *cs, const LEX_CSTRING &str,
                                    bool casedn)
  {
    casedn ? copy_casedn(cs, str) : copy_bin(str);
    return *this;
  }
  explicit operator const LEX_CSTRING () const
  {
    return LEX_CSTRING({m_buff, m_length});
  }
  const LEX_CSTRING to_lex_cstring() const
  {
    return LEX_CSTRING({m_buff, m_length});
  }
  const char *str() const { return m_buff; }
  size_t length() const { return m_length; }
};

#endif // CHAR_BUFFER_INCLUDED
