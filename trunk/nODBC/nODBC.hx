/*
 * Copyright (c) 2007, DesignRealm.co.uk
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
 * 
 * Written by Lee McColl Sylvester
 *
 */

import neko.db.Connection;

private class ODBCResultSet implements ResultSet {

	public var length(getLength,null) : Int;
	public var nfields(getNFields,null) : Int;
	var r : Void;
	var cache : Dynamic;

	public function new( r ) {
		this.r = r;
	}

	function getLength() {
		return result_get_length( r );
	}

	function getNFields() {
		return result_get_nfields( r );
	}

	public function hasNext() {
		if( cache == null )
			cache = next();
		return ( cache != null );
	}

	public function next() : Dynamic {
		var c = cache;
		if( c != null ) {
			cache = null;
			return c;
		}
		c = result_next( r );
		if( c == null )
			return null;
		untyped {
			var f = __dollar__objfields( c );
			var i = 0;
			var l = __dollar__asize( f );
			while( i < l ) {
				var v = __dollar__objget( c, f[i] );
				if( __dollar__typeof( v ) == __dollar__tstring )
					__dollar__objset( c, f[i], new String( v ) );
				i = i + 1;
			}
		}
		return c;
	}

	public function results() : List<Dynamic> {
		var l = new List();
		while( hasNext() )
			l.add( next() );
		return l;
	}

	public function getResult( n : Int ) {
		return new String( result_get( r, n ) );
	}

	public function getIntResult( n : Int ) : Int {
		return result_get_int( r, n );
	}

	public function getFloatResult( n : Int ) : Float {
		return result_get_float( r, n );
	}

	static var result_next = neko.Lib.load("nODBC","nodbc_result_next",1);
	static var result_get_length = neko.Lib.load("nODBC","nodbc_result_get_length",1);
	static var result_get_nfields = neko.Lib.load("nODBC","nodbc_result_get_nfields",1);
	static var result_get = neko.Lib.load("nODBC","nodbc_result_get",2);
	static var result_get_int = neko.Lib.load("nODBC","nodbc_result_get_int",2);
	static var result_get_float = neko.Lib.load("nODBC","nodbc_result_get_float",2);

}

private class ODBCConnection implements Connection {

	private var __c : Void;
	public var dbname(getDBName,setDBName) : String;
	public var lastInsertId : Void->Int;
	private var _dbname : String;
	private var id : Int;

	public function new( c ) {
		_dbname = "ODBC";
		__c = _connect( c, null, null );
	}

	public function request( qry : String ) : ResultSet {
		var r = _request( __c, untyped qry.__s );
		var rs : ResultSet = new ODBCResultSet( r );
		id = lastInsertId();
		return rs;
	}
	
	function setDBName( s : String )
	{
		_dbname = s;
		var me = this;
		switch( s.toLowerCase() )
		{
			case "mssql":
				lastInsertId = function()
				{
					var rset : ODBCResultSet = cast( me.request( "SELECT @@IDENTITY" ), ODBCResultSet );
					return rset.getIntResult(0);
				}
			case "mysql":
				lastInsertId = function()
				{
					var rset : ODBCResultSet = cast( me.request( "SELECT LAST_INSERT_ID()" ), ODBCResultSet );
					return rset.getIntResult(0);
				}
			default:
				lastInsertId = function()
				{
					return 0;
				}
		}
		return s;
	}
	
	function getDBName() : String
	{
		return _dbname;
	}

	public function close() {
		_close( __c );
	}

	public function escape( s : String ) {
		return s.split( "\\" ).join( "\\\\" ).split( "'" ).join( "\\'" );
	}

	public function quote( s : String ) {
		return "'"+escape( s )+"'";
	}

	public function dbName() {
		return _dbname;
	}

	public function startTransaction() {
		request("BEGIN TRANSACTION");
	}

	public function commit() {
		request("COMMIT");
	}

	public function rollback() {
		request("ROLLBACK");
	}

	public function hasFeature( f ) {
		/*switch( f ) 
		{
			case ForUpdate: return false;
		}*/
		return false;
	}

	static var _connect = neko.Lib.load("nODBC","nodbc_connect",3);
	static var _close = neko.Lib.load("nODBC","nodbc_free_connection",1);
	static var _request = neko.Lib.load("nODBC","nodbc_request",2);
}

class nODBC {

	public static function open( conn : String, dbName : String ) : Connection {
		var c = new ODBCConnection( untyped conn.__s );
		cast( c, ODBCConnection ).dbname = dbName;
		return c;
	}

}
