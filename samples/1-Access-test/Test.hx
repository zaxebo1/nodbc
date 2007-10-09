class Test
{
	static function main() {
		var cnx = neko.db.ODBC.open("Driver={Microsoft Access Driver (*.mdb)};Dbq=test.mdb;Uid=;Pwd=;", "Access");
		cnx.lastInsertId = function()
		{
			return 0;
		}

		cnx.request("CREATE TABLE tblCustomers (
			CustomerID autoincrement,
			LastName TEXT(50) NOT NULL, 
			FirstName TEXT(50) NOT NULL, 
			Phone TEXT(10), 
			Email TEXT(50) )
		");
		neko.Lib.print( cnx.request("INSERT INTO tblCustomers ([LastName],[FirstName],Phone,Email) VALUES ('Smith', 'John', '1234', 'john@smith.com')") );
		neko.Lib.print( cnx.request("INSERT INTO tblCustomers ([LastName],[FirstName],Phone,Email) VALUES ('Bishop', 'James', '4321', 'james@thekid.com')") );

		var rs = cnx.request("SELECT * FROM tblCustomers");
		for( row in rs )
		{
		    neko.Lib.print( "User "+row.FirstName+" "+row.LastName+" can be called on "+row.Phone+" or email "+row.Email+"\n" );
		}
		cnx.close();
	}
}