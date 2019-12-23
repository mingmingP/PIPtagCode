#!/usr/bin/ruby
#
#
#

require 'csv';

bbe = `which bbe`.chomp
floatconv = 'fc'
floatconv_src = 'floatToBits.cpp'

date = `date +%s`
tmpfile = "tmp#{date}".chomp
logfile = "#{tmpfile}.log"

id_key="\\xba\\xda\\xba\\xab"
off_key="\\xda\\xda\\xda\\xad"
slope_key="\\xca\\xda\\xca\\xac"

await_input = 1

def usage 
	puts "Parameters: PIP_IMAGE TEMP_FILE START END"
	puts "  PIP_IMAGE - Binary output file from CCS"
	puts "  TEMP_FILE - CSV containing temperature data"
	puts "  START - First PIP ID"
	puts "  END - Last PIP ID"
end

def failedImage
  puts "+------------------------+"
  puts "|                        |"
  puts "|    FLASHING FAILURE    |"
  puts "|                        |"
  puts "+------------------------+"
  puts ""
  puts "Press any key to try again"
  puts "or [Ctrl]+C to quit."
  readline
end

def replaceHex(filename, search, replace) 
end

###################################

unless ARGV.length == 4
	usage
	exit 1
end

infile = ARGV[0]
temp_offset_file = ARGV[1]
start_idx = ARGV[2].to_i
end_idx = ARGV[3].to_i

tempData = Hash.new

CSV.foreach(temp_offset_file) do |row|
	id,slope,offset = row
	tempData[id] = slope, offset
end


puts "Using temporary file #{tmpfile}"

index = start_idx
until index > end_idx
	new_id=sprintf("%08x",index)

	if await_input == 1
		puts "Please attach PIP #{index}"
		puts "Press [ENTER] to continue or 's' to skip"
		response = STDIN.readline.chomp
		if response == 's'
			index += 1
			next
		end
	end
	await_input = 1

	id1=new_id[0,2]
	id2=new_id[2,2]
	id3=new_id[4,2]
	id4=new_id[6,2]
	bbecmd = "s/#{id_key}/\\x#{id4}\\x#{id3}\\x#{id2}\\x#{id1}/"
	`#{bbe} -o #{tmpfile} -e "#{bbecmd}" #{infile}`
	success=$?.success?

	unless success
		failedImage
		next
	end

	slope,offset = tempData[index.to_s]
	unless slope == nil
		puts "Setting slope: #{slope}, and offset: #{offset}"

		slope_long = `./fc #{slope}`
		offset_long = `./fc #{offset}`
		id1=slope_long[0,2]
		id2=slope_long[2,2]
		id3=slope_long[4,2]
		id4=slope_long[6,2]
		
		bbecmd = "s/#{slope_key}/\\x#{id4}\\x#{id3}\\x#{id2}\\x#{id1}/"
		`#{bbe} -o #{tmpfile}.0 -e "#{bbecmd}" #{tmpfile}`

		success = $?.success?


		unless success
			failedImage
			next
		end

		id1=offset_long[0,2]
		id2=offset_long[2,2]
		id3=offset_long[4,2]
		id4=offset_long[6,2]

		bbecmd = "s/#{off_key}/\\x#{id4}\\x#{id3}\\x#{id2}\\x#{id1}/"
		`#{bbe} -o #{tmpfile} -e "#{bbecmd}" #{tmpfile}.0`

		success = $?.success?
		unless success
			failedImage
			next
		end

	end # No slope/offset data available
	puts "Flashing PIP #{index}"
	output = `mspdebug rf2500 "prog #{tmpfile}" 2>&1`
	success = $?.success?
	unless success
		if output.include? "usbutil: unable to find a device matching 0451:f432"
			puts "USB device changed. Retrying flash"
			await_input = 0
			next
		end
		failedImage
		next
	end


	puts "Verifying PIP #{index}"
	`mspdebug rf2500 "verify #{tmpfile}" 2>&1`
	success = $?.success?
	unless success
		failedImage
		next
	end

	puts "#######################"
	puts "# FLASHING SUCCESSFUL #"
	puts "#######################"
	puts ""

	`rm #{tmpfile} #{tmpfile}.0`


	index += 1
end 
