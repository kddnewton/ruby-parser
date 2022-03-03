# frozen_string_literal: true

require "open3"
require "ripper"
require "test/unit"

class TokenizeTest < Test::Unit::TestCase
  script = File.expand_path("../build/parse", __dir__)
  fixture = File.expand_path("fixtures/tokenize.rb", __dir__)

  File.foreach(fixture, chomp: true).with_index(1) do |line, index|
    next if line.empty?

    define_method(:"test_line_#{index}") do
      source, expected = line.split(" # ")

      stdout, status = Open3.capture2("#{script} tokenize", stdin_data: source)
      actual = stdout.chomp.tr("\n", " ")

      assert_equal(0, status, "Expected parse to exit cleanly")
      assert_equal(expected, actual, "Expected to match the comment")

      actual = lex(source).join(" ")

      assert_equal(expected, actual, "Expected to match the ripper output")
    end
  end

  private

  def lex(source)
    line_counts = []
    last_index = 0

    source.lines.each do |line|
      line_counts << last_index
      last_index += line.size
    end

    Ripper.lex(source).map do |token|
      token => [[lineno, column], type, value, _state]

      "%d-%d %s %s" % [
        start_char = line_counts[lineno - 1] + column,
        end_char = line_counts[lineno - 1] + column + value.length,
        type.to_s[3..-1],
        value
      ]
    end
  end
end
