build/parse: src/*.c src/encoding/*.c src/visitor/*.c src/*.h
	mkdir -p build
	cc -o build/parse -Wall -Wextra -Isrc src/*.c src/encoding/*.c src/visitor/*.c

FORCE:

parse: FORCE build/parse test.rb
	build/parse parse test.rb

tokenize: FORCE build/parse test.rb
	build/parse tokenize test.rb

test: FORCE build/parse test/*.rb
	ruby test/runner.rb
