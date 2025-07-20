def print_result(result, label: nil)
  puts "\n--- #{label} ---" if label
  puts "#{result}"
  result.each_with_index do |line, index|
    next if line.start_with?("[")

    if line.start_with?("db>")
      puts "#{index + 1}: \e[32m#{line}\e[0m"
    else
      puts "#{index + 1}: #{line}"
    end
  end
end

def run_script(commands)
  raw_output = nil
  IO.popen("make run", "r+") do |pipe|
    commands.each do |command|
      pipe.puts command
    end

    pipe.close_write

    # Read entire output
    raw_output = pipe.gets(nil)
  end
  raw_output.split("\n")
end


def contains(result, text)
  match = result.any? { |line| line.include?(text) }
  expect(match).to be true
end


describe 'Simple sanity tests' do
  before(:all) do
    # Ensure the database is clean before running tests
    system("make clean")
  end
  #
  # Test cases for the REPL meta commands
  #
  it 'prints usage information' do
    result = run_script([".help", ".exit"])
    contains(result, "not implemented")
  end

  it 'prints error message for unknown command' do
    result = run_script([".unknown", ".exit"])
    contains(result, "unrecognized meta command")
  end

  it 'prints goodbye message on exit' do
    result = run_script([".exit"])
    contains(result, "goodbye")
  end
  #
  # Test simple commands
  #
  it 'prints command sizing error for invalid insert' do
    result = run_script(["insert", ".exit"])
    contains(result, "COMMAND_SIZING_ERR")
  end

  it 'prints command syntax error for invalid insert' do
    result = run_script(["insert -1 foo bar", ".exit"])
    contains(result, "COMMAND_SYNTAX_ERR")
  end

  it 'prints command unknown error for invalid command' do
    result = run_script(["unknown 1", ".exit"])
    contains(result, "COMMAND_UNKNOWN")
  end

  it 'prints ok for select' do
    result = run_script(["select", ".exit"])
    contains(result, "handling command")
  end

  it 'prints ok for insert' do
    result = run_script(["insert 1 foo bar", ".exit"])
    contains(result, "handling command")
  end

  it 'prints ok for update' do
    result = run_script(["update", ".exit"])
    contains(result, "handling command")
  end

  it 'prints ok for delete' do
    result = run_script(["delete", ".exit"])
    contains(result, "handling command")
  end
end

describe 'Simple DB operations' do
  before(:all) do
    # Ensure the database is clean before running tests
    system("make clean")
  end
  # 
  # Insert/Select commands
  #
  it 'prints adds table row on insert' do
    result = run_script(["insert 1 foo bar", "select", ".exit"])
    contains(result, "(1, foo, bar)")
  end

  it 'prints adds table row on multiple inserts' do
    result = run_script(["insert 2 foo bar", "insert 3 bar foo", "select", ".exit"])
    contains(result, "(2, foo, bar)")
    contains(result, "(3, bar, foo)")
  end

  it 'prints row with max column size values' do
    long_username = "a"*32
    long_email = "a"*255
    script = ["insert 4 #{long_username} #{long_email}", "select", ".exit"]
    result = run_script(script)
    contains(result, "(4, #{long_username}, #{long_email})")
  end

  it 'prints error message if strings are too long' do
    long_username = "a"*33
    long_email = "a"*256
    script = ["insert 1 #{long_username} #{long_email}", "select", ".exit"]
    result = run_script(script)
    contains(result, "username or email exceeds maximum length")
    contains(result, "COMMAND_SIZING_ERR")
  end

  it 'prints error message for duplicate insert' do
    result = run_script(["insert 1 foo bar", "insert 1 baz qux", "select", ".exit"])
    contains(result, "Duplicate key error")
    contains(result, "(1, foo, bar)")
  end
end

describe 'Btree operations' do
  before(:all) do
    # Ensure the database is clean before running tests
    system("make clean")
  end
  it 'allows printing out the structure of a 4-leaf-node btree' do
    script = [
      "insert 1 foo bar",
      "insert 2 bar foo",
      "insert 3 baz qux",
      "insert 4 quux corge",
      "insert 5 grault garply",
      "insert 6 waldo fred",
      "insert 7 plugh xyzzy",
      "insert 8 thud wibble",
      "insert 9 wobble wobble",
      "insert 10 flob flib",
      "insert 11 blip blop",
      "insert 12 blop blip",
      "insert 13 foo2 bar2",
      "insert 14 bar2 foo2",
      "insert 15 baz2 qux2",
      "insert 16 quux2 corge2",
      ".btree",
      "select",
      ".exit",
    ]
    result = run_script(script)
    contains(result, "leaf (size 7)")
    contains(result, "leaf (size 9)")
    contains(result, "(1, foo, bar)") 
    contains(result, "(2, bar, foo)")
    contains(result, "(3, baz, qux)")
    contains(result, "(4, quux, corge)")
    contains(result, "(5, grault, garply)")
    contains(result, "(6, waldo, fred)")
    contains(result, "(7, plugh, xyzzy)")
    contains(result, "(8, thud, wibble)")
    contains(result, "(9, wobble, wobble)")
    contains(result, "(10, flob, flib)")
    contains(result, "(11, blip, blop)")
    contains(result, "(12, blop, blip)")
    contains(result, "(13, foo2, bar2)")
    contains(result, "(14, bar2, foo2)")
    contains(result, "(15, baz2, qux2)")
    contains(result, "(16, quux2, corge2)")
  end
end

describe 'Complex operation' do
  before(:all) do
    # Ensure the database is clean before running tests
    system("make clean")
  end
  it 'allows printing out the structure of a 4-leaf-node btree2' do
    script = [
      "insert 18 user18 person18@example.com",
      "insert 7 user7 person7@example.com",
      "insert 10 user10 person10@example.com",
      "insert 29 user29 person29@example.com",
      "insert 23 user23 person23@example.com",
      "insert 4 user4 person4@example.com",
      "insert 14 user14 person14@example.com",
      "insert 30 user30 person30@example.com",
      "insert 15 user15 person15@example.com",
      "insert 26 user26 person26@example.com",
      "insert 22 user22 person22@example.com",
      "insert 19 user19 person19@example.com",
      "insert 2 user2 person2@example.com",
      "insert 1 user1 person1@example.com",
      "insert 21 user21 person21@example.com",
      "insert 11 user11 person11@example.com",
      "insert 6 user6 person6@example.com",
      "insert 20 user20 person20@example.com",
      "insert 5 user5 person5@example.com",
      "insert 8 user8 person8@example.com",
      "insert 9 user9 person9@example.com",
      "insert 3 user3 person3@example.com",
      "insert 12 user12 person12@example.com",
      "insert 27 user27 person27@example.com",
      "insert 17 user17 person17@example.com",
      "insert 16 user16 person16@example.com",
      "insert 13 user13 person13@example.com",
      "insert 24 user24 person24@example.com",
      "insert 25 user25 person25@example.com",
      "insert 28 user28 person28@example.com",
      ".btree",
      ".exit",
    ]
    result = run_script(script)
    # print_result(result, label: "Btree Structure")
    contains(result, "leaf (size 7)")
    contains(result, "leaf (size 8)")
  end
end
