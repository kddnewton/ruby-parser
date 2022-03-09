build/parse: build/libparse.dylib src/cli/*.c
	cc -o build/parse build/libparse.dylib -Wall -Wextra -Isrc src/cli/*.c

build/libparse.dylib: src/*.c src/encoding/*.c src/*.h
	mkdir -p build
	cc --shared -o build/libparse.dylib -Wall -Wextra -Isrc src/*.c src/encoding/*.c

FORCE:

parse: FORCE build/parse test.rb
	build/parse parse test.rb

tokenize: FORCE build/parse test.rb
	build/parse tokenize test.rb

test: FORCE build/parse test/*.rb
	ruby test/runner.rb
