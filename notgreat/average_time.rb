#!/usr/bin/ruby

FILE_NAME="debug.txt"


count=0
total=0.0
temp =0.0
max=0.0
min=3000.0

File.open(FILE_NAME, 'r') do |file|
	
	file.each_line{ |line| 
		line = line.split(" ")

		if line.length < 2
			next
		end

		if ( line[0].match('setup')   )
			temp = line[1].to_f			
		elsif ( line[0].match('tactics') )
			time_taken= line[1].to_f - temp
			total+=time_taken
			max = time_taken if time_taken >max
			min = time_taken if time_taken < min
			count+=1
		end
	}
	puts "Average: " + (total/count).to_s
	puts "Max: " + max.to_s
	puts "Min: " + min.to_s
end
