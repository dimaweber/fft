#include "stdafx.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

#include "mysql_driver.h"
#include "mysql_connection.h"

#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_complex_math.h>

int main(int argc, const char **argv)
{
	try {
		size_t n = 128;
		std::vector<double> last_rate(n);
		std::vector<double> last_rate_orig(n);
		/*
		sql::mysql::MySQL_Driver* driver = nullptr;
		sql::Connection* connection = nullptr;

		driver = sql::mysql::get_mysql_driver_instance();
		connection = driver->connect("tcp://mysql.dweber.lan:3306/trade", "trader", "traderpassword");

		sql::Statement* statement = connection->createStatement();
		sql::ResultSet* result = statement->executeQuery("select time, last_rate from rates where currency='usd' and goods='btc' limit 128 ");

		size_t size = result->rowsCount();
		size_t l = floor(log(size) / log (2));
		size_t size_fixed = gsl_pow_uint(2, l);
		if (size_fixed < size)
			size_fixed *= 2;

		int index = size_fixed - size;

		while (result->next())
		{
			last_rate[index++] = result->getDouble("last_rate");
		}
		for (index = 0; index < size_fixed - size; index++)
			last_rate[index] = last_rate[size-1];

		result->close();
		statement->close();
		connection->close();
		*/

		double T = 4;
		for (size_t k = 0; k < n; k++)
		{
			double t = (T / n) * k;
			last_rate_orig[k] = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t) + 0.25 * sin(2.5 * 2 * M_PI * t);
			last_rate[k]      = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t) + 0.25 * sin(2.5 * 2 * M_PI * t);
		}


		int res = gsl_fft_real_radix2_transform(last_rate.data(), 1, n);
		if (res != GSL_SUCCESS)
		{
			std::cout << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
			return 1;
		}

		std::vector<double> complex(n*2);
		res = gsl_fft_halfcomplex_radix2_unpack(last_rate.data(), complex.data(), 1, n);
		if (res != GSL_SUCCESS)
		{
			std::cout << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
			return 1;
		}
		else
			for (int k = 0; k < n/2; k++)
			{
				gsl_complex c;
				GSL_SET_COMPLEX(&c, complex[2 * k], complex[2 * k + 1]);
				double t = T / n * k;
				double amplitude = gsl_complex_abs(c) * 2 / n;
				if (amplitude > 0.0001)
				std::cout << k << ';'
						  << k / T   << ';' 
						  << amplitude << ';' 
						  << gsl_complex_arg(c) << std::endl;
			}

		
		/*
		delete result;
		delete statement;
		delete connection;
		delete driver;
		*/
	}
	catch (sql::SQLException &e) {
		/*
		MySQL Connector/C++ throws three different exceptions:

		- sql::MethodNotImplementedException (derived from sql::SQLException)
		- sql::InvalidArgumentException (derived from sql::SQLException)
		- sql::SQLException (derived from std::runtime_error)
		*/
		std::cout << "# ERR: SQLException in " << __FILE__;
		std::cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << std::endl;
		/* what() (derived from std::runtime_error) fetches error message */
		std::cout << "# ERR: " << e.what();
		std::cout << " (MySQL error code: " << e.getErrorCode();
		std::cout << ", SQLState: " << e.getSQLState() << " )" << std::endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}