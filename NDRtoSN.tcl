namespace eval NDRtoSN {

    proc controls {} {
	radiobox NDRtoSN::format "output format" ".lsn/.hsn .h" "-l -c" -c
    }

    proc command {} {
    
	return "NDRtoSN $NDRtoSN::format"
    }

}


