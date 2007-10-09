/*
 * Copyright (c) 2007, Lee McColl Sylvester - www.designrealm.co.uk
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE HAXE PROJECT CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE HAXE PROJECT CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/callablestatement.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/types.h>

#include <sstream>
#include <iostream>
#include <memory>

#include "neko.h"

#define ODBCCONN( o )		((Connection*)val_data( o ))
#define ODBCRESULT( o )		((result*)val_data( o ))

DEFINE_KIND( k_connection );
DEFINE_KIND( k_result );

using namespace std;
using namespace odbc;

#undef CONV_FLOAT

typedef enum {
	CONV_INT,
	CONV_STRING,
	CONV_FLOAT,
	CONV_BINARY,
	CONV_DATE,
	CONV_DATETIME,
	CONV_BOOL
} CONV;

typedef struct {
	ResultSet *r;
	Statement *s;
	int nfields;
	int nrows;
	CONV *fields_convs;
	char* *fields_names;
	field *fields_ids;
	int current;
	value conv_date;
} result;

static CONV convert_type( Types::SQLType t, unsigned int length ) {
	switch( t )
	{
		case Types::SMALLINT:
		case Types::TINYINT:
			return CONV_INT;
		case Types::BINARY:
		case Types::CHAR:
		case Types::LONGVARBINARY:
		case Types::LONGVARCHAR:
		case Types::VARBINARY:
		case Types::VARCHAR:
		case Types::WCHAR:
		case Types::WLONGVARCHAR:
		case Types::WVARCHAR:
			return CONV_STRING;
		case Types::BIT:
			return CONV_BOOL;
		case Types::DATE:
			return CONV_DATE;
		case Types::TIME:
		case Types::TIMESTAMP:
			return CONV_DATETIME;

		case Types::BIGINT:
		case Types::INTEGER:

		case Types::DECIMAL:
		case Types::DOUBLE:
		case Types::FLOAT:
		case Types::NUMERIC:
		case Types::REAL:
			return CONV_FLOAT;
		default:
			//printf("string\n");
			return CONV_STRING;
	}
}

static value nodbc_free_connection( value c )
{
	val_check_kind( c, k_connection );
	val_gc( c, NULL );
	ODBCCONN( c )->~Connection();
}

static value nodbc_connect( value url, value user, value password )
{
    value v;
	Connection* con;
	
	val_check( url, string );

	try
	{
		if ( user == val_null || password == val_null )
			con = DriverManager::getConnection( ODBCXX_STRING( val_string( url ) ) );
		else
			con = DriverManager::getConnection( ODBCXX_STRING( val_string( url ) ), ODBCXX_STRING( val_string( user ) ), ODBCXX_STRING( val_string( password ) ) );
	} catch(SQLException& e) {
		buffer b = alloc_buffer( "Connection to database failed: " );
		buffer_append( b, e.getMessage().c_str() );
		bfailure( b );
	}
    con->setAutoCommit(false);

	v = alloc_abstract( k_connection, con );
	return v;
}

static value nodbc_result_next( value o )
{
	result *r;
	unsigned long *lengths = NULL;
	val_check_kind(o,k_result);
	r = ODBCRESULT(o);
	char *buf;
	int i;
	value cur = alloc_object(NULL);
	if ( o == val_null ) return val_null;
	if ( !r->r->next() ) return val_null;
	if ( r->current == NULL ) r->current = 0;
	for(i=0;i<r->nfields;i++)
	{
		value v;
		switch( r->fields_convs[i] ) {
		case CONV_INT:
			v = alloc_int((int)r->r->getInt(i+1));
			break;
		case CONV_STRING:
			v = alloc_string(r->r->getString(i+1).c_str());
			break;
		case CONV_BOOL:
			v = alloc_bool( r->r->getBoolean(i+1) );
			break;
		case CONV_FLOAT:
			v = alloc_float(r->r->getDouble(i+1));
			break;
		case CONV_BINARY:
			v = alloc_string(r->r->getString(i+1).c_str());
			break;
		case CONV_DATE:
			if( r->conv_date == NULL )
				v = alloc_string(r->r->getDate(i+1).toString().c_str());
			else {
				struct tm t;
				sscanf(r->r->getDate(i+1).toString().c_str(),"%4d-%2d-%2d",&t.tm_year,&t.tm_mon,&t.tm_mday);
				t.tm_hour = 0;
				t.tm_min = 0;
				t.tm_sec = 0;
				t.tm_isdst = -1;
				t.tm_year -= 1900;
				t.tm_mon--;						
				v = val_call1(r->conv_date,alloc_int32((int)mktime(&t)));
			}
			break;
		case CONV_DATETIME:
			if( r->conv_date == NULL )
				v = alloc_string(r->r->getDate(i+1).toString().c_str());
			else {
				struct tm t;
				sscanf(r->r->getDate(i+1).toString().c_str(),"%4d-%2d-%2d %2d:%2d:%2d",&t.tm_year,&t.tm_mon,&t.tm_mday,&t.tm_hour,&t.tm_min,&t.tm_sec);
				t.tm_isdst = -1;
				t.tm_year -= 1900;
				t.tm_mon--;
				v = val_call1(r->conv_date,alloc_int32((int)mktime(&t)));
			}
			break;
		default:
			v = val_null;
			break;
		}
		alloc_field(cur,r->fields_ids[i],v);
	}
	r->current++;
	return cur;
}

/**
	result_get : 'result -> n:int -> string
	<doc>Return the [n]th field of the current row</doc>
**/
static value nodbc_result_get( value o, value n ) {
	result *r;
	const char *s;
	val_check_kind(o,k_result);
	val_check(n,int);
	r = ODBCRESULT(o);
	if( val_int(n) < 0 || val_int(n) >= r->nfields )
		neko_error();
	if( r->current == NULL )
		r->current++;
	s = r->r->getString(val_int(n)).c_str();
	return alloc_string( s?s:"" );
}

/**
	result_get_int : 'result -> n:int -> int
	<doc>Return the [n]th field of the current row as an integer (or 0)</doc>
**/
static value nodbc_result_get_int( value o, value n ) {
	result *r;
	int s;
	val_check_kind(o,k_result);
	val_check(n,int);
	r = ODBCRESULT(o);
	if( val_int(n) < 0 || val_int(n) >= r->nfields )
		neko_error();
	if( r->current == NULL )
		r->current++;
	s = r->r->getLong(val_int(n));
	return alloc_int( s );
}

/**
	result_get_float : 'result -> n:int -> float
	<doc>Return the [n]th field of the current row as a float (or 0)</doc>
**/
static value nodbc_result_get_float( value o, value n ) {
	result *r;
	double s;
	val_check_kind(o,k_result);
	val_check(n,int);
	r = ODBCRESULT(o);
	if( val_int(n) < 0 || val_int(n) >= r->nfields )
		neko_error();
	if( r->current == NULL )
		r->current++;
	s = r->r->getDouble(val_int(n));
	return alloc_float( s );
}

/**
	result_set_conv_date : 'result -> function:1 -> void
	<doc>Set the function that will convert a Date or DateTime string
	to the corresponding value.</doc>
**/
static value nodbc_result_set_conv_date( value o, value c )
{
	result *res;
	val_check_function( c, 1 );
	if( val_is_int( o ) )
		return val_true;
	val_check_kind( o, k_result );
	res = ODBCRESULT( o );
	res->conv_date = c;
	return val_true;
}

static value alloc_result( ResultSet* r, Statement* s )
{
	result *res = (result*)alloc( sizeof( result ) );
	value o = alloc_abstract( k_result, res );
	ResultSetMetaData* rsmd = r->getMetaData();
	int num_fields = rsmd->getColumnCount();
	int num_rows = 0;
	int i,j;
	res->r = r;
	res->s = s;
	res->conv_date = NULL;
	res->current = NULL;
	res->nfields = num_fields;
	res->nrows = num_rows;
	res->fields_ids = (field*)alloc_private(sizeof(field)*num_fields);
	res->fields_names = (char**)alloc_private(sizeof(char*)*num_fields);
	res->fields_convs = (CONV*)alloc_private(sizeof(CONV)*num_fields);
	for( i=0; i<num_fields; i++ )
	{
		field id = val_id( rsmd->getColumnName( i+1 ).c_str() );
		for( j=0; j<i; j++ )
		{
			if( res->fields_ids[j] == id ) {
				buffer b = alloc_buffer("Error, same field ids for : ");
				buffer_append( b, rsmd->getColumnName( i+1 ).c_str() );
				buffer_append( b, ":" );
				val_buffer( b, alloc_int( i+1 ) );
				buffer_append( b, " and " );
				buffer_append( b, rsmd->getColumnName( i+1 ).c_str() );
				buffer_append( b, ":" );
				val_buffer( b, alloc_int( j+1 ) );
				buffer_append( b, "." );
				bfailure( b );
			}
		}
		res->fields_ids[i] = id;
		res->fields_names[i] = (char*)rsmd->getColumnName( i+1 ).c_str();
		res->fields_convs[i] = convert_type( (Types::SQLType)rsmd->getColumnType( i+1 ) , rsmd->getPrecision( i+1 ) );
	}
	//val_gc( o, np_free_result );
	return o;
}

static value nodbc_request( value c, value r )
{
	ResultSet* rs;
	Statement* stmt;
	value v;

	val_check_kind( c, k_connection );
	val_check( r, string );

	try
	{
		stmt = ODBCCONN( c )->createStatement
			(ResultSet::TYPE_SCROLL_INSENSITIVE, ResultSet::CONCUR_READ_ONLY);

		stmt->setFetchSize(10);

		printf( ODBCXX_STRING( ODBCCONN( c )->nativeSQL( val_string( r ) ) ).c_str() );

		if( stmt->execute( ODBCXX_STRING( ODBCCONN( c )->nativeSQL( val_string( r ) ) ) ) )
		{
			rs = stmt->getResultSet();

			return alloc_result( rs, stmt );
		} else {
			stmt->getUpdateCount();
			ODBCCONN( c )->commit();
			return val_null;
		}
	} catch(SQLException& e) {
		buffer b = alloc_buffer( "Query request failed: " );
		buffer_append( b, e.getMessage().c_str() );
		bfailure( b );
	}
}

/* return the number of rows for a given result object */
static value nodbc_result_get_length( value m )
{
	return alloc_int( 0 );
}

/* return the number of columns for a given result object */
static value nodbc_result_get_nfields( value m )
{
	result *res;

	val_check_kind( m, k_result );

	res = ODBCRESULT( m );

	return alloc_int( res->r->getMetaData()->getColumnCount() );
}

static value nodbc_result_get_column_name( value m, value c )
{
	result *res;

	val_check_kind( m, k_result );
	val_check( c, int );

	res = ODBCRESULT( m );

	return alloc_string( res->r->getMetaData()->getColumnName( val_int( c ) ).c_str() );
}

static value nodbc_result_get_column_number( value m, value c )
{
	result *res;

	val_check_kind( m, k_result );
	val_check( c, string );

	res = ODBCRESULT( m );

	for( int i=1; i<=res->nfields; i++ )
	{
		if ( res->r->getMetaData()->getColumnName( i ) == val_string( c ) )
		{
			return alloc_string( res->r->getMetaData()->getColumnName( i ).c_str() );
		}
	}
	return val_null;
}

void* p_nodbc_connect = (void*)nodbc_connect;
void* p_nodbc_free_connection = (void*)nodbc_free_connection;
void* p_nodbc_request = (void*)nodbc_request;

void* p_nodbc_result_get_column_name = (void*)nodbc_result_get_column_name;
void* p_nodbc_result_get_column_number = (void*)nodbc_result_get_column_number;
void* p_nodbc_result_get_length = (void*)nodbc_result_get_length;
void* p_nodbc_result_get_nfields = (void*)nodbc_result_get_nfields;
void* p_nodbc_result_next = (void*)nodbc_result_next;
void* p_nodbc_result_get = (void*)nodbc_result_get;
void* p_nodbc_result_get_int = (void*)nodbc_result_get_int;
void* p_nodbc_result_get_float = (void*)nodbc_result_get_float;
void* p_nodbc_result_set_conv_date = (void*)nodbc_result_set_conv_date;

DEFINE_PRIM(p_nodbc_connect,3);
DEFINE_PRIM(p_nodbc_free_connection,1);
DEFINE_PRIM(p_nodbc_request,2);

DEFINE_PRIM(p_nodbc_result_get_column_name,2);
DEFINE_PRIM(p_nodbc_result_get_column_number,2);
DEFINE_PRIM(p_nodbc_result_get_length,1);
DEFINE_PRIM(p_nodbc_result_get_nfields,1);
DEFINE_PRIM(p_nodbc_result_next,1);
DEFINE_PRIM(p_nodbc_result_get,2);
DEFINE_PRIM(p_nodbc_result_get_int,2);
DEFINE_PRIM(p_nodbc_result_get_float,2);
DEFINE_PRIM(p_nodbc_result_set_conv_date,2);

//DEFINE_PRIM(nodbc_free_result,1);
//DEFINE_PRIM(nodbc_last_insert_id,1);
//DEFINE_PRIM(nodbc_reset_connection,1);
