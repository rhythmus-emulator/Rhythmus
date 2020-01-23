#pragma once

#include <assert.h>
#include <stdexcept>
#include <string>

#define ASSERT(x) assert(x)
#define ASSERT_M(x, s) while (!(x)) { throw std::runtime_error(s); break; }
#define ASSERT_GL() ASSERT(glGetError() == 0)
#define ASSERT_GL_VAL(x) ASSERT((x = glGetError()) == 0)

namespace rhythmus
{

/* @brief
 * Base runtime exception. used for fatal exception. program will be closed.
 * @param msg_ description about exception
 * @param is_ignorable_ is this exception can be ignored and continue? */
class RuntimeException : public std::exception
{
public:
  RuntimeException(const std::string& msg);
  virtual const char* exception_name() const;
  virtual const char* what() const;
  void set_ignorable(bool ignorable);
  bool is_ignorable() const;
private:
  std::string msg_;
  bool is_ignorable_;
};

/* @brief Used for unimplemented feature (fatal exception) */
class UnimplementedException : public RuntimeException
{
public:
  UnimplementedException(const std::string& msg);
  virtual const char* exception_name() const;
};

/* @brief Used when this exception can be retryable or fatal. */
class RetryException : public RuntimeException
{
public:
  RetryException(const std::string& msg);
  virtual const char* exception_name() const;
};

/* @brief Used when required file is not found. */
class FileNotFoundException : public RuntimeException
{
public:
  FileNotFoundException(const std::string& filename);
  virtual const char* exception_name() const;
};

/* @brief ASSERT for rhythmus solution.
 * RuntimeException will be thrown if false. */
void R_ASSERT(bool v);

/* @brief ASSERT with message */
void R_ASSERT(bool v, const char *msg);
void R_ASSERT(bool v, const std::string &msg);

}