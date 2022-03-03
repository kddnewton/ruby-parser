# frozen_string_literal: true

require "open3"
require "ripper"
require "stringio"
require "test/unit"

class ParseTest < Test::Unit::TestCase
  class Parser < Ripper::SexpBuilder
    def initialize(...)
      super
      @output = StringIO.new
    end

    def on_args_new
      []
    end

    def on_args_add(args, arg)
      args << arg
    end

    def on_array(contents)
      @output.puts("ARRAY=#{contents&.length || 0}")
      super
    end

    def on_aref(left, right)
      @output.puts(right ? "INDEX" : "INDEX_CALL")
      super
    end

    alias on_aref_field on_aref

    def on_assign(left, right)
      @output.puts("ASSIGN")
      super
    end

    def on_backref(value)
      prefix = value.match?(/\A\$\d+\z/) ? "NTH" : "BACK"
      @output.puts("#{prefix}_REFERENCE=#{value}")
      super
    end

    def on_begin(bodystmt)
      @output.puts("BEGIN")
      super
    end

    def on_binary(left, oper, right)
      @output.puts(
        case oper
        in :"||" then "LOGICAL_OR"
        in :"&&" then "LOGICAL_AND"
        in :<=> then "COMPARE"
        in :== then "DOUBLE_EQUAL"
        in :=== then "TRIPLE_EQUAL"
        in :"!=" then "BANG_EQUAL"
        in :"=~" then "EQUAL_TILDE"
        in :"!~" then "BANG_TILDE"
        in :+ then "ADD"
        in :- then "SUBTRACT"
        in :* then "MULTIPLY"
        in :/ then "DIVIDE"
        in :% then "MODULO"
        in :** then "EXPONENT"
        in :and then "COMPOSITION_AND"
        in :or then "COMPOSITION_OR"
        end
      )
      super
    end

    def on_defined(value)
      @output.puts("DEFINED")
      super
    end

    def on_dot2(left, right)
      @output.puts(left ? "RANGE_INCLUSIVE" : "BEGINLESS_RANGE_INCLUSIVE")
      super
    end

    def on_dot3(left, right)
      @output.puts(left ? "RANGE_EXCLUSIVE" : "BEGINLESS_RANGE_EXCLUSIVE")
      super
    end

    def on_fcall(ident)
      @output.puts("FCALL=#{ident[1]}")
      super
    end

    def on_gvar(value)
      @output.puts("GLOBAL_VARIABLE=#{value}")
      super
    end

    def on_ifop(pred, left, right)
      @output.puts("TERNARY")
      super
    end

    def on_if_mod(pred, stmt)
      @output.puts("IF_MODIFIER")
      super
    end

    def on_int(value)
      @output.puts("INTEGER=#{value}")
      super
    end

    def on_opassign(left, oper, right)
      @output.puts(
        case oper[1]
        in "+=" then "ADD_ASSIGN"
        in "-=" then "SUBTRACT_ASSIGN"
        in "*=" then "MULTIPLY_ASSIGN"
        in "/=" then "DIVIDE_ASSIGN"
        in "%=" then "MODULO_ASSIGN"
        in "&=" then "BITWISE_AND_ASSIGN"
        in "|=" then "BITWISE_OR_ASSIGN"
        in "^=" then "BITWISE_XOR_ASSIGN"
        in "&&=" then "LOGICAL_AND_ASSIGN"
        in "||=" then "LOGICAL_OR_ASSIGN"
        in "<<=" then "SHIFT_LEFT_ASSIGN"
        in ">>=" then "SHIFT_RIGHT_ASSIGN"
        in "**=" then "EXPONENT_ASSIGN"
        end
      )
      super
    end

    def on_program(stmts)
      @output
    end

    def on_rescue_mod(left, right)
      @output.puts("RESCUE_MODIFIER")
      super
    end

    def on_unary(oper, value)
      @output.puts(
        case oper
        in :+ then "UPLUS"
        in :-@ then "UMINUS"
        in :"!" then "UBANG"
        in :~ then "UTILDE"
        in :not then "NOT"
        end
      )
      super
    end

    def on_unless_mod(pred, stmt)
      @output.puts("UNLESS_MODIFIER")
      super
    end

    def on_until_mod(pred, stmt)
      @output.puts("UNTIL_MODIFIER")
      super
    end

    def on_until(pred, stmts)
      @output.puts("UNTIL")
      super
    end

    def on_var_ref(value)
      case value
      in [:@kw, "false", _] then @output.puts("FALSE")
      in [:@kw, "nil", _] then @output.puts("NIL")
      in [:@kw, "self", _] then @output.puts("SELF")
      in [:@kw, "true", _] then @output.puts("TRUE")
      else
      end
      super
    end

    def on_vcall(ident)
      @output.puts("VCALL=#{ident[1]}")
      super
    end

    def on_while(pred, stmts)
      @output.puts("WHILE")
      super
    end

    def on_while_mod(pred, stmt)
      @output.puts("WHILE_MODIFIER")
      super
    end
  end

  script = File.expand_path("../build/parse", __dir__)
  fixture = File.expand_path("fixtures/parse.rb", __dir__)

  File.foreach(fixture, chomp: true).with_index(1) do |line, index|
    next if line.empty?

    define_method(:"test_line_#{index}") do
      source, expected = line.split(" # ")

      stdout, status = Open3.capture2("#{script} parse", stdin_data: source)
      actual = stdout.chomp.tr("\n", " ")

      assert_equal(0, status, "Expected parse to exit cleanly")
      assert_equal(expected, actual, "Expected to match the comment")

      output = Parser.parse(source)
      actual = output.string.chomp.tr("\n", " ")

      assert_equal(expected, actual, "Expected to match the ripper output")
    end
  end
end
