/*
 *  This file is part of Empirical, https://github.com/devosoft/Empirical
 *  Copyright (C) Michigan State University, MIT Software license; see doc/LICENSE.md
 *  date: 2024-2025.
*/
/**
 *  @file
 */


#include "emp/base/notify.hpp"
#include "emp/base/vector.hpp"
#include "emp/compiler/Lexer.hpp"
#include "emp/io/CPPFile.hpp"
#include "emp/tools/String.hpp"
#include "emp/web/web.hpp"

#include "TokenInput.hpp"

namespace UI = emp::web;

class Emplex {
private:
  static constexpr size_t MAX_TOKENS = 100;

  UI::Document doc{"emp_base"};
  emp::CPPFile file;
  emp::vector<emp::String> errors;
  emp::Lexer lexer;

  // Lexer information
  emp::vector<TokenInput> token_info;
  size_t active_token = 1024;

  // Lexer C++ Output Configuration
  emp::String out_filename{"lexer.hpp"};
  emp::String lexer_name{"Lexer"};
  emp::String token_name{"Token"};
  emp::String dfa_name{"DFA"};
  emp::String inc_guards{"EMPLEX_LEXER_HPP_INCLUDE_"};
  emp::String name_space{"emplex"};
  bool use_token_lexemes = true;
  bool use_token_line_num = true;
  bool use_token_column = true;

  // Sections of web page
  UI::Div intro_div{"intro_div"};
  UI::Div button_div{"button_div"};
  UI::Div token_div{"token_div"};
  UI::Div settings_div{"settings_div"};
  UI::Div error_div{"error_div"};
  UI::Div sandbox_div{"sandbox_div"};
  UI::Div output_div{"output_div"};
  UI::Div footer_div{"footer_div"};

  UI::Table token_table{1, 4, "token_table"};
  UI::Table settings_table{15, 3, "settings_table"};
  UI::Text output_text{"output_text"};
  UI::TextArea sandbox_input{"sandbox_input"};
  UI::Text sandbox_text{"sandbox_text"};

  // Sandbox state
  bool sandbox_show_ignore = false;        // Should we show token types marked as "ignore"?
  bool sandbox_show_token_info = false;    // Should we isolate tokens with brackets and show more info?
  bool sandbox_show_types = false;         // Should we show the type of each token?
  bool sandbox_show_lines = false;         // Should we show the line number of each token?
  emp::vector<emp::String> sandbox_colors; // Foreground colors to use for tokens
  emp::vector<emp::String> sandbox_bgs;    // Background colors to use for tokens

  std::string highlight_color = "#ddddff";

  UI::Style button_style {
    "padding", "10px 15px",
    "background-color", "#000066",   // Dark Blue background
    "color", "white",                // White text
    "border", "1px solid white",     // Thin white border
    "border-radius", "5px",          // Rounded corners
    "cursor", "pointer",
    "font-size", "16px",
    "transition", "background-color 0.3s ease, transform 0.3s ease" // Smooth transition
  }; 

  UI::Style table_style {
    "background-color", "white",
    "color", "white",
    "padding", "10px",
    "border", "1px solid black",
    "text_align", "center"
  };

  UI::Style div_style {
    "border-radius", "10px",
    "border",        "1px solid black",
    "padding",       "15px",
    "width",         "800px",
    "margin-top",    "10pt"
  };

  UI::Style sandbox_but_style {
    "padding", "5px 10px",
    "background-color", "#220022",   // Dark Purple background
    "color", "white",                // White text
    "border", "1px solid white",     // Thin white border
    "border-radius", "5px",          // Rounded corners
    "cursor", "pointer",
    "font-size", "12px",
    "transition", "background-color 0.3s ease, transform 0.3s ease" // Smooth transition
  }; 

  // ---- HELPER FUNCTIONS ----

  void UpdateErrors() {
    if (errors.size()) {
      output_text.Clear();
      output_div.Redraw();
      doc.Button("download_but").SetBackground("#606060").SetDisabled().SetTitle("Generate code to activate this button.");
    }

    error_div.Clear();
    for (emp::String & error : errors)  {
      error_div << emp::MakeWebSafe(error) << "<br>\n";
    }
    error_div.Redraw();
  }

  // Highlight a specified table row.
  void ActivateTableRow(size_t row_id) {
    if (active_token < token_info.size()) {
      token_info[active_token].GetNameWidget().SetBackground("white");
      token_info[active_token].GetRegexWidget().SetBackground("white");
    }
    token_info[row_id].GetNameWidget().SetBackground(highlight_color);
    token_info[row_id].GetRegexWidget().SetBackground(highlight_color);
    active_token = row_id;
  }

  // Set up the callback functions for a table row (or RESET a row if line numbers change)
  void SetupTableRowCallbacks(size_t row_id) {
    token_info[row_id].GetNameWidget().SetCallback([this](std::string){
      GenerateLexer();
      UpdateSandbox();
    });
    token_info[row_id].GetRegexWidget().SetCallback([this](std::string){
      GenerateLexer();
      UpdateSandbox();
    });
    token_info[row_id].GetIgnoreWidget().SetCallback([this](bool){
      GenerateLexer();
      UpdateSandbox();
    });
    token_info[row_id].GetRemoveButton().SetCallback([this, row_id](){
      RemoveTableRow(row_id); doc.Div("token_div").Redraw();
    });
  }

  // Add a row to the bottom of the token table.
  void AddTableRow() {
    size_t token_id = token_table.GetNumRows() - 1; // Top row is labels, not token
    if (token_id >= MAX_TOKENS) {
      emp::notify::Warning("Maximum ", MAX_TOKENS, " token types allowed!");
      return;
    }
    auto new_row = token_table.AddRow();
    new_row.OnMouseDown([this,token_id](){ ActivateTableRow(token_id); });

    emp_assert(token_id <= token_info.size());
    // Grow the table if we need to.
    if (token_id == token_info.size()) {
      // emp::notify::Message("Token_id = ", token_id, "; token_info.size() = ", token_info.size());
      // token_info.emplace_back(TokenInput(token_id));
      token_info.emplace_back(token_id);
      SetupTableRowCallbacks(token_id);
    }     
    auto & row_info = token_info[token_id];
    new_row[0] << row_info.GetNameWidget();
    new_row[1] << row_info.GetRegexWidget();
    new_row[2] << "&nbsp;&nbsp;&nbsp;" << row_info.GetIgnoreWidget();
    new_row[3] << row_info.GetRemoveButton();
  }

  void AddTableRow(emp::String name, emp::String regex, bool ignore=false) {
    const size_t row_id = token_table.GetNumRows()-1;
    AddTableRow();
    token_info[row_id].Set(name, regex, ignore);
  }

  bool SwapTableRows(size_t row1, size_t row2) {
    [[maybe_unused]] const size_t num_rows = token_table.GetNumRows() - 1;
    if (row1 >= num_rows || row2 >= num_rows) return false; // No place to move to.

    token_info[row1].Swap(token_info[row2]);
    return true;
  }

  // Remove a specified row from the table.
  void RemoveTableRow(size_t id) {
    const size_t num_rows = token_table.GetNumRows() - 1;
    emp_assert(id < num_rows);  // Make sure row to be deleted actually exists.
    // Swap rows to move deleted row to the end.
    while (id < num_rows-1) {
      SwapTableRows(id, id+1);
      ++id;
    }
    // Remove last row
    token_info[id].Clear();
    auto new_row = token_table.RemoveRow();
  }

  // Remove all rows in the table.
  void ClearTable() {
    for (auto & row : token_info) row.Clear();
    token_table.Rows(1);
  }

  void SaveTable() {
    emp::String out;

    for (const auto & t_info : token_info) {
      emp::String name = t_info.GetName();
      emp::String regex = t_info.GetRegex();
      bool ignore = t_info.GetIgnore();
      if (name.empty()) continue;  // Unnamed token types can be skipped.

      if (ignore) out += "-";
      out += name + ' ' + regex + '\n';
    }

    emp::DownloadFile("lexer.emplex", out);
  }

  template <typename... Ts>
  void Error(size_t line_num, Ts &&... args) {
    errors.push_back(emp::MakeString("Error (line ", line_num, ") - ", args...));
  }

  // Make sure that the token table contains only valid information.
  bool TestValidTable() {
    // Make sure all of the token information is valid.
    errors.resize(0);
    std::unordered_set<emp::String> token_names;
    for (size_t line_num = 0; line_num < token_info.size(); ++line_num) {
      auto & t_info = token_info[line_num];
      emp::String name = t_info.GetName();
      emp::String regex = t_info.GetRegex();
  
      if (name.empty() && regex.empty()) continue;  // Completely empty slots can be skipped.      

      if (name.empty()) {
        Error(line_num, "No name provided for RegEx: ", regex);
        continue;
      }
      else if (regex.empty()) {
        Error(line_num, "No regex provided for token '", name, "'");
        continue;
      }

      if (!name.OnlyIDChars()) {
        Error(line_num, "Invalid token name '", name, "'; only letters, digits, and '_' allowed.");
        continue;
      }
      if (token_names.contains(name)) {
        Error(line_num, "Multiple token types named '", name, "'.");
        continue;
      }
      token_names.insert(name);

      emp::RegEx regex_text(regex);
      for (const auto & note : regex_text.GetNotes()) {
        Error(line_num, "Invalid Regular expression: ", note);
        continue;
      }
    }

    // Halt generation if any errors were triggered.
    UpdateErrors();
    return !errors.size();
  }

  /// Try to generate a lexer based on the current regular expressions.
  /// @return success
  bool GenerateLexer() {
    if (!TestValidTable()) return false;

    lexer.Reset();

    // Load all of the tokens ino the lexer (since they passed tests, assume they are valid)
    for (const auto & t_info : token_info) {
      emp::String name = t_info.GetName();
      emp::String regex = t_info.GetRegex();
      bool ignore = t_info.GetIgnore();

      if (name.empty()) continue;

      if (ignore) lexer.IgnoreToken(name, regex);
      else lexer.AddToken(name, regex);
    }

    return true;
  }

  /// Try to generate CPP code based on the current regular expressions.
  /// @return success
  bool GenerateCPP() {
    if (!GenerateLexer()) return false;

    file.Clear();
    file.SetGuards(inc_guards);
    file.SetNamespace(name_space);
    lexer.WriteCPP(file, lexer_name, dfa_name, token_name,
                   use_token_lexemes, use_token_line_num, use_token_column);

    std::stringstream ss;
    file.Write(ss);
    output_text.Clear();
    output_text.SetBorder("20px");
    output_text << "<pre style=\"padding: 10px; border-radius: 5px; overflow-x: auto;\">\n" << emp::MakeWebSafe(ss.str()) << "\n</pre>\n";
    output_div.Redraw();

    doc.Button("download_icon").Hide(false);
    doc.Button("copy_icon").Hide(false);
    doc.Button("close_icon").Hide(false);

    doc.Button("download_but").SetDisabled(false).SetBackground("#330066").SetTitle("Click to download the generated code.");

    return true;
  }

  void ClearOutput() {
    output_text.Clear();
    doc.Button("download_icon").Hide();
    doc.Button("copy_icon").Hide();
    doc.Button("close_icon").Hide();
    output_div.Redraw();
  }

  void CopyCode() {
    std::stringstream ss;
    file.Write(ss);
    emp::CopyText(ss.str());
    doc.Button("copy_icon").SetLabel("<img src=\"Icons/ICON-Copied.png\" width=\"50px\">").Redraw();
    emp::DelayCall([this](){
      doc.Button("copy_icon").SetLabel("<img src=\"Icons/ICON-Copy.png\" width=\"50px\">").Redraw();
    }, 1500);
  }

  void DownloadCode() {
    std::stringstream ss;
    file.Write(ss);
    emp::DownloadFile(out_filename, ss.str());
  }

  void ToggleSandbox() {
    sandbox_div.ToggleActive();
    if (sandbox_div.IsInactive()) return;
  }

  emp::String HeadingName(emp::String name) {
    return emp::MakeString("<big><big><b>", name, "</b></big></big><br>\n");
  }

  emp::String MakeLink(emp::String text, emp::String link) {
    return emp::MakeHTMLLink(text, link, "#C0C0FF");
  }

  emp::String MakeTrigger(emp::String text, std::function<void()> fun) {
    return emp::MakeHTMLTrigger(text, fun, "#C0C0FF");
  }

  // Load a set of example regular expressions into the lexer input.
  void LoadExampleLexer() {
    ClearTable();
    AddTableRow("whitespace", "[ \\t\\n\\r]+", true);
    AddTableRow("comment",    "#.*", true);
    AddTableRow("integer",    "[0-9]+");
    AddTableRow("float",      "([0-9]+\\.[0-9]*)|(\\.[0-9]+)");
    AddTableRow("keyword",    "(break)|(continue)|(else)|(for)|(if)|(return)|(while)");
    AddTableRow("type",       "(char)|(double)|(int)|(string)");
    AddTableRow("identifier", "[a-zA-Z_][a-zA-Z0-9_]*");
    // AddTableRow("string",     "(\\\"([^\"\\\\]|(\\\\.))*\\\")|('([^'\\\\]|(\\\\.))*')");
    AddTableRow("operator",     "\"::\"|\"==\"|\"!=\"|\"<=\"|\">=\"|\"->\"|\"&&\"|\"||\"|\"<<\"|\">>\"|\"++\"|\"--\"");
    doc.Div("token_div").Redraw();
  }

  void UpdateIntro(emp::String mode) {
    intro_div.Clear();
    const emp::String text_color = "white";
    const emp::String active_color = "#0000AA";
    const emp::String button_color = "#000044";
    const emp::String table_color = "white"; // "#FFFFE0";
    intro_div.SetColor(text_color).SetBackground(button_color).SetCSS(div_style);
    doc.Button("home_but").SetBackground(button_color);
    doc.Button("lexer_but").SetBackground(button_color);
    doc.Button("regex_but").SetBackground(button_color);
    doc.Button("cpp_but").SetBackground(button_color);
    doc.Button("example_but").SetBackground(button_color);
    doc.Button("about_but").SetBackground(button_color);

    if (mode == "home") {
      doc.Button("home_but").SetBackground(active_color);
      static emp::String trigger_text =
        MakeTrigger("load an example", [this](){ LoadExampleLexer(); });
      intro_div << HeadingName("Overview") <<
          "<p>Emplex uses a set of <b>token names</b> and associated <b>regular expressions</b> to "
          "generate C++ code for a fast, table-driven lexer for ASCII input. "
          "Click on the buttons above to learn more or " << trigger_text << ".</p>"; 
    } else if (mode == "lexer") {
      doc.Button("lexer_but").SetBackground(active_color);
      intro_div << HeadingName("Lexical analysis") <<
        "<p>A " << MakeLink("lexical analyzer", "https://en.wikipedia.org/wiki/Lexical_analysis") <<
        " (commonly called a \"lexer\", \"tokenizer\", or \"scanner\") reads a stream of input "
        "characters, typically from a text file, and breaks it into tokens that each form an "
        "atomic input unit.  For example, if we consider the following code where we might be "
        "calculating the area of a triangle:</p>\n"
        "<p>&nbsp;&nbsp;<code style=\"background-color: " << table_color << "; color: black; padding:10px; border: 1px solid black\">double area3 = base * height / 2.0;</code></p>"
        "<p>We could convert this statement into the series of tokens:</p>"
        "<p><table cellpadding=2px border=2px style=\"background-color: " << table_color << "; color: black; text-align: center;\">"
        "<tr><th width=150px>Lexeme</th><th width=150px>Token Type</th></tr>"
        "<tr><td><code>double</code></td> <td>TYPE</td>       </tr>"
        "<tr><td><code>area3</code></td>  <td>IDENTIFIER</td> </tr>"
        "<tr><td><code>=</code></td>      <td>OPERATOR</td>   </tr>"
        "<tr><td><code>base</code></td>   <td>IDENTIFIER</td> </tr>"
        "<tr><td><code>*</code></td>      <td>OPERATOR</td>   </tr>"
        "<tr><td><code>height</code></td> <td>IDENTIFIER</td> </tr>"
        "<tr><td><code>/</code></td>      <td>OPERATOR</td>   </tr>"
        "<tr><td><code>2.0</code></td>    <td>FLOAT</td>      </tr>"
        "<tr><td><code>;</code></td>      <td>ENDLINE</td>    </tr>"
        "</table></p>\n"
        "<p>In order to build a lexer, we define the set of token types that we want to use and "
        "build a <i>regular expression</i> for each that can identify the associated tokens.</p>\n"
        "<p>The lexer will always find the <i>longest</i> token that can be fully matched from the "
        "beginning of the input. If there is a tie for longest, the lexer will match the <i>first</i> "
        "token type listed.</p>\n"
        "<p>For example, we could define the following token types:</p>\n"
        "<p><table cellpadding=2px border=2px style=\"background-color: " << table_color << "; color: black; text-align: center;\">\n"
        "  <tr><td width=150px>KEYWORD</td> <td width=200px><code>(for)|(if)|(set)|(while)</code></td></tr>\n"
        "  <tr><td>IDENTIFIER</td>          <td><code>[a-zA-Z_][a-zA-Z0-9_]*</code></td>              </tr>\n"
        "  <tr><td>INTEGER   </td>          <td><code>[0-9]+                </code></td>              </tr>\n"
        "  <tr><td>WHITESPACE</td>          <td><code>[ \\t\\n\\r]          </code></td>              </tr>\n"
        "</table></p>\n"
        "<p>Then if we were parsing \"<code>set formula_id 5</code>\", "
        "the first token would be \"set\" and it would be type KEYWORD because while both "
        "KEYWORD and IDENTIFIER could match this series fo characters, KEYWORD comes first in the list. "
        "The next token would be a single space of type WHITESPACE, though if we marked the "
        "WHITESPACE token as 'ignore' then its characters would be skipped over and the token would "
        "not be included in the returned vector. "
        "After that the characters \"for\" could be matched by KEYWORD, but IDENTIFIER would be able "
        "to match the longer \"formula_id\", and as such it would be chosen next.<p>\n"
        "<p>See the next tab if you want to learn about writing regular expressions in Emplex.</p>\n";
    } else if (mode == "regex") {
      doc.Button("regex_but").SetBackground(active_color);
      intro_div << HeadingName("Regular Expressions") <<
        "<p>A " << MakeLink("regular expression", "https://en.wikipedia.org/wiki/Regular_expression") <<
        " (or \"regex\") is a mechanism to describe a pattern of characters "
        "and, in particular, they can be used to describe tokens for lexical analysis.</p> "
        "<p>In a regular expression, letters and digits always directly match themselves, but other "
        "characters often have a special function.  The following regular expression techniques are "
        "implemented in Emplex (a subset of the regex rules that were used in GNU's " <<
        MakeLink("Flex", "https://ftp.gnu.org/old-gnu/Manuals/flex-2.5.4/html_mono/flex.html#SEC7") <<
        "):</p>\n"

        "<p><table border=\"2\" cellpadding=\"3\" style=\"background: white; color: black\">\n"
        "<tr><th>Symbol</th> <th>Description</th><th>Example</th><th>Explanation</th>\n"
        "<tr><th>|</th>      <td>A logical \"or\" (match just one side)</td>"
                            "<td><code>this|that</code></td>"
                            "<td>Match the words \"this\" or \"that\", but nothing else</td>\n"
        "<tr><th>( ... )</th> <td>Specify grouping</td>"
                            "<td><code>th(is|at)</code></td>"
                            "<td>Also match just the words \"this\" or \"that\"</td>\n"
        "<tr><th>\"</th>     <td>Quotes (directly match symbols inside)</td>"
                            "<td><code>\"|\"</code></td>"
                            "<td>Match the pipe symbol</td>\n"
        "<tr><th>?</th>      <td>The previous match is optional</td>"
                            "<td><code>a?b</code></td>"
                            "<td>Match \"ab\" or just \"b\"; the 'a' is optional</td>\n"
        "<tr><th>*</th>      <td>The previous match can be made zero, one, or multiple times</td>"
                            "<td><code>c*d</code></td>"
                            "<td>Match \"d\", \"cd\", \"ccd\", \"cccccd\" or with any other number of c's</td>\n"
        "<tr><th>+</th>      <td>The previous match can be made one or more times</td>"
                            "<td><code>(ab)+</code></td>"
                            "<td>Match \"ab\", \"abab\", \"ababababab\", with any non-zero number of ab's</td>\n"
        "<tr><th>{n}</th>    <td>The previous entry must be matched exactly n times</td>"
                            "<td><code>\"Beetlejuice\"{3}</code></td>"
                            "<td>Match the string \"Beetlejuice\" exactly three times in a row.</td>\n"
        "<tr><th>{n,}</th>   <td>The previous entry must be matched at least n times, but any"
                            "    number of matches are allowed.</td>"
                            "<td><code>(0|1){10,}</code></td>"
                            "<td>Match at least 10 bits, but any larger number of bits is"
                            "    allowed.</td>\n"
        "<tr><th>{m,n}</th>  <td>The previous entry must be matched at least m times, but no"
                            "    more than n times.</td>"
                            "<td><code>A{3,5}</code></td>"
                            "<td>Match \"AAA\", \"AAAA\", or \"AAAAA\".</td>\n"
        "<tr><th>[ ... ]</th> <td>Match any single character between the brackets; ranges of characters are allowed using a dash ('-'). If the first character is a caret ('^') match any character EXCEPT those listed.</td>"
                            "<td><code>[0-9]</code></td>"
                            "<td>Match any single digit.</td>\n"
        "</table></p>\n"

        "<p>We also have many different shortcuts that can be used inside of a regular expression:</p>\n"
        "<p><table border=\"2\" cellpadding=\"3\" style=\"background: white; color: black\">\n"
        "<tr><th>Shortcut</th> <th>Expansion</th>      <th>Meaning</th> </tr></tr>\n"
        "<tr><th>.</th>        <td><code>[^\\n]</code> <td>Match any single character <i>except</i> a newline ('\\n')</td></tr>\n"
        "<tr><th>\\d</th>      <td><code>[0-9]</code>  <td>Match any single digit</td></tr>\n"
        "<tr><th>\\D</th>      <td><code>[^0-9]</code> <td>Match any single non-digit character</td></tr>\n"
        "<tr><th>\\l</th>      <td><code>[a-zA-Z]</code>  <td>Match any single letter</td></tr>\n"
        "<tr><th>\\L</th>      <td><code>[^a-zA-Z]</code> <td>Match any single non-letter character</td></tr>\n"
        "<tr><th>\\s</th>      <td><code>[ \\f\\n\\r\\t\\v]</code> <td>Match any single whitespace character</td></tr>\n"
        "<tr><th>\\S</th>      <td><code>[^\\f\\n\\r\\t\\v]</code> <td>Match any single non-whitespace character</td></tr>\n"
        "<tr><th>\\w</th>      <td><code>[A-Za-z0-9_]</code>  <td>Match any identifier (\"word\") character</td></tr>\n"
        "<tr><th>\\W</th>      <td><code>[^A-Za-z0-9_]</code> <td>Match any single non-identifier character</td></tr>\n"
        "</table></p>\n"

        "<p>Here are some examples of regular expression techniques:</p>\n"
        "<p><table border=\"2\" cellpadding=\"3\" style=\"background: white; color: black\">\n"
        "<tr><td><code>.*</code></td> <td>Match all characters until the end of the current line.</td></tr>\n"
        "<tr><td><code>\"if\"|\"while\"|\"for\"</code></td> <td>Match common keywords.</td></tr>\n"      
        "<tr><td><code>x0[0-9a-fA-F]+</code></td> <td>Match hexadecimal values</td></tr>\n"
        "<tr><td><code>(http(s?)\"://\")?\\w+([./]\\w+)+</code></td> <td>A simple URL matcher</td></tr>\n"      
        "</table></p>\n"

        "<p>Note that traditionally regular expressions will pick the FIRST match that's "
        "possible, but a lexer uses a principle called " <<
        MakeLink("maximal munch", "https://en.wikipedia.org/wiki/Maximal_munch") << 
        " which means that it will always take the LONGEST match it can find.</p>\n"
        ;
    } else if (mode == "cpp") {
      doc.Button("cpp_but").SetBackground(active_color);
      intro_div << HeadingName("Working with the Generated C++ Code") <<
        "<p>Emplex will generate a C++ file that you can either copy or download "
        "(as \"lexer.hpp\" by default) and simply <code>#include</code> into your own code. "
        "The generated file will contain a lexer class (called \"Lexer\" by default) in a namespace "
        "(\"emplex\" by default). "
        "Create a lexer object and run Tokenize() on input text to convert it to a vector of Tokens.</p>\n"
        "<p>For example, if you are making a lexer for the language \"Cabbage\" and want to tokenize "
        "\"mycode.cab\", you could write:</p>\n"
        "<pre style=\"background-color: " << table_color << "; color: black; padding:10px\">\n"
        "   std::ifstream in_file(\"mycode.cab\");    // Load the input file\n"
        "   emplex::Lexer lexer;                    // Build the lexer object\n"
        "   const std::vector&lt;emplex::Token&gt; & tokens = lexer.Tokenize(in_file);\n"
        "   // ... Use the vector of tokens ...\n"
        "</pre>\n"
        "<p>Each token is a simple <code>struct</code>:</p>\n"
        "<pre style=\"background-color: " << table_color << "; color: black; padding:10px\">\n"
        "   struct Token {\n"
        "     int id;              // Type ID for this token\n"
        "     std::string lexeme;  // Sequence of chars matched by this token\n"
        "     size_t line_id;      // Line this token started on\n"
        "     size_t col_id;       // Column this token started on\n"
        "   };\n"
        "</pre>\n"
        "<p>The <code>id</code> value for a token will indicate its type and "
        "will either match one of the \"ID_\" values defined in the lexer, "
        "or it will be an ASCII code (for a single-character token with a default match.) "
        "For example, if the source file has the number 100, the token's lexeme would be "
        "\"100\" and it's ID would be the value of <code>emplex::ID_INT</code>.</p>"
        "The <code>line_id</code> and <code>col_id</code> fields give the position in the file "
        "where the token was found, which can be useful for error reporting.</p>\n"
        "<p>Finally, if you need a token's type name, you can use: "
        "<code>emplex::Lexer::TokenName(token);</code></p>"

        "<p>Once an input is tokenized, you can manage tokens one at a time in the lexer. "
        "The following member functions are available:</p>"
        "<table style=\"color: black; background-color: white; padding: 10px; border: 1px solid black\">\n"
        "<tr><th style=\"width: 35%\">Name <th>Usage </tr>"
        "<tr><td><code>bool Any()</code>"
        "    <td>Are there any tokens remaining to be processed? </tr>"
        "<tr><td><code>bool Is(int type_id)</code>"
        "    <td>Does the current token have the provided type ID? </tr>"
        "<tr><td><code>Token Peek()</code>"
        "    <td>Get the current token, but do NOT advance </tr>"
        "<tr><td><code>Token Use()</code>"
        "    <td>Get the current token and advance. </tr>"
        "<tr><td><code>Token Use(int type_id, [message])</code>"
        "    <td>Use the current token; give an error if is not the expected type (custom error message is optional)</tr>"
        "<tr><td><code>bool UseIf(int type_id, ...)</code>"
        "    <td>Use the current token only if it is one of the provided types; return the token's type ID if was used or 0 if it remains unsused.</tr>"
        "<tr><td><code>void Rewind(int steps=1)</code>"
        "    <td>Mark a previous token as current again.</tr>"
        "</table>"
        "<p>For example, you might have a <code>ParseStatement()</code> function that looks something like this:</p>"
        "<pre style=\"background-color: " << table_color << "; color: black; padding:10px\">\n"
        " ASTNode * Parse_Statement() {\n"
        "   // Determine statement type by first token and call appropriate parse function\n"
        "   switch (tokens.Peek()) {\n"
        "     using namespace emplex;\n"
        "     case Lexer::ID_TYPE:   return Parse_Statement_Declare();\n"
        "     case Lexer::ID_IF:     return Parse_Statement_If();\n"
        "     case Lexer::ID_WHILE:  return Parse_Statement_While();\n"
        "     case Lexer::ID_RETURN: return Parse_Statement_Return();\n"
        "     // ...cases for other statement types like 'break', 'continue', etc...\n"
        "     case '{': return Parse_StatementList();\n"
        "     case ';':       // Empty line of code.\n"
        "       tokens.Use(); // Move past the semicolon.\n"
        "       return nullptr;\n"
        "     default: // Assume anything else is an expression.\n"
        "       return Parse_Statement_Expression();\n"
        "   }\n"
        "  }\n"
        "</pre>"
        "<p>Parsing a particular type of statement might look like:</p>"
        "<pre style=\"background-color: " << table_color << "; color: black; padding:10px\">\n"
        "  ASTNode * Parse_Statement_If() {\n"
        "    using namespace emplex;\n"
        "    auto if_token = tokens.Use(Lexer::ID_IF);\n"
        "    tokens.Use('(', \"If commands must be followed by a '('\");\n"
        "    ASTNode * condition = Parse_Expression();\n"
        "    tokens.Use(')', \"Missing ')' at end of if condition.\");\n"
        "    ASTNode * action = Parse_Statement();\n"
        "\n"
        "    // If we have an 'else' branch, parse it.\n"
        "    ASTNode * alt = tokens.UseIf(Lexer::ID_ELSE) ? Parse_Statement() : nullptr\n"
        "\n"
        "    return MakeASTNodeIf(condition, action, alt);\n"
        "  }\n"
        "</pre>"
        "<br><br>";
    } else if (mode == "about") {
      doc.Button("about_but").SetBackground(active_color);
      intro_div << HeadingName("About") <<
        "<p>Emplex is written in C++ using the "
        << MakeLink("Empirical Library", "https://github.com/devosoft/Empirical") <<
        " and then compiled into "
        << MakeLink("WebAssembly", "https://webassembly.org/") << " with the "
        << MakeLink("Emscripten", "https://emscripten.org/") << " LLVM compiler.</p>"
        "<p>Emplex takes in a set of token types and associated regular expressions. "
        "Each regular expression is then converted into a non-deterministic finite automaton (NFA). "
        "The set of automata are merged together, while tracking which token type each end "
        "condition is associated with. When an end condition could have come from two different "
        "regular expressions, the regex listed first (highest in the list) is used. "
        "The resulting NFA is converted into a DFA and then implemented as a table. "
        "That table is hard-coded in the generated C++ output, along with "
        "associated helper functions.  When tokenization is performed, the longest possible input "
        "string is matched and the ID associated with that end condition is returned.</p>"
        "<p>The Emplex software and most of the associated tools in the underlying "
        "Empirical library were written by:<br><br> "
        << "<b>" << MakeLink("Dr. Charles Ofria", "https://ofria.com/") << "</b><br>"
        << MakeLink("Michigan State University", "https://msu.edu/") << "<br>"
        "<a href=\"mailto:ofria@cse.msu.edu\" style=\"color: #C0C0FF\">ofria@cse.msu.edu</a><br><br>\n"

        "<a href=\"https://scholar.google.com/citations?user=nYLuKDAAAAAJ\" target=\"_blank\" rel=\"noopener noreferrer\">"
        "  <img src=\"https://img.shields.io/badge/Google%20Scholar-Follow-blue?style=social&logo=google-scholar\" alt=\"Follow on Google Scholar\">"
        "</a><br>  "
        "<a href=\"https://bsky.app/profile/ofria.bsky.social\">"
        "  <img src=\"https://img.shields.io/badge/Bluesky-0285FF?logo=bluesky&logoColor=fff&label=@ofria\" alt=\"Follow on Bluesky\">"
        "</a><br>"
        "<a href=\"https://github.com/mercere99\" target=\"_blank\" rel=\"noopener noreferrer\">"
        "  <img src=\"https://img.shields.io/github/followers/mercere99?label=Github&style=social\" alt=\"Follow on GitHub\">"
        "</a><br>"
        "<br>"
        
        "<h3>My current To-Do list for the site includes:</h3>\n"
        "<ul>\n"
        "<li>Set up alternate languages to generate to, including Python, Java, C, and Rust.</li>"
        "<li>Colorize code examples on 'Generated C++ Code' page.</li>"
        "<li>Allow more output customization in the advanced settings, including turning off helper functions.</li>"
        "<li>When saving token type information, also save advanced settings for easy restore.</li>"
        "<li>Make {alias} tokens that work as aliases only in Lexers.  For example, if you define a token as {abc} it won't be used for matching, but you can put {abc} inside of another regular expression to include it.</li>"
        "<li>Make RegExA/RegExB work (match RegExA if and only if RegExB follows; RegExB counts toward length.)</li>"
        "<li>Follow up with Empala, a Parser generator!</li>"
        "</ul>\n"
        "<br><br>"
        ;
    }
  }

  void InitializeButtonDiv() {
    button_div << UI::Button([this](){
      UpdateIntro("home"); intro_div.Redraw(); 
    }, "Home", "home_but").SetCSS(button_style).SetBackground("#0000AA").SetCSS("width", "159px");
    button_div << UI::Button([this](){
      UpdateIntro("lexer"); intro_div.Redraw(); 
    }, "Lexical Analysis", "lexer_but").SetCSS(button_style); // .SetCSS("width", "170px");
    button_div << UI::Button([this](){
      UpdateIntro("regex"); intro_div.Redraw(); 
    }, "Regular Expressions", "regex_but").SetCSS(button_style); // .SetCSS("width", "170px");
    button_div << UI::Button([this](){
      UpdateIntro("cpp"); intro_div.Redraw(); 
    }, "Generated C++ Code", "cpp_but").SetCSS(button_style); // .SetCSS("width", "170px");
    button_div << UI::Button([this](){
      UpdateIntro("about"); intro_div.Redraw(); 
    }, "About", "about_but").SetCSS(button_style).SetCSS("width", "159px");
  }

  void InitializeTokenDiv() {
    // token_table.SetCSS("border-collapse", "collapse");
    token_div.SetBackground("lightgrey").SetCSS("margin-top", "10pt", "border-radius", "10px", "border", "1px solid black", "padding", "15px", "width", "800px");
    token_div << HeadingName("Token Types");

    token_table.SetColor("#000044");
    token_table.GetCell(0,0).SetHeader() << "Token Name";
    token_table.GetCell(0,1).SetHeader() << "Regular Expression";
    token_table.GetCell(0,2).SetHeader() << "Ignore?";

    // Start table with three rows?
    AddTableRow();
    AddTableRow();
    AddTableRow();

    token_div << token_table;

    token_div << "<p>";
    token_div << UI::Button([this](){
      AddTableRow();
      doc.Div("token_div").Redraw();
    }, "Add Row", "row_but").SetCSS(button_style)
    .SetTitle("Add an additional line for defining token types.");

    token_div << UI::Button([this](){
      AddTableRow();
      AddTableRow();
      AddTableRow();
      AddTableRow();
      AddTableRow();
      doc.Div("token_div").Redraw();
    }, "+5 Rows", "5row_but").SetCSS(button_style)
    .SetTitle("Add five more lines for defining additional tokens.");

    token_div << UI::Button([this](){
      ClearTable();
      AddTableRow();
      AddTableRow();
      AddTableRow();
      doc.Div("token_div").Redraw();
    }, "Reset", "reset_but").SetCSS(button_style)
    .SetTitle("Reset tokens back to the starting setup.");

    token_div << UI::Button([this](){ SaveTable(); }, "Save Token Types")
      .SetCSS(button_style)
      .SetTitle("Save token names and regular expressions to a file.");

    token_div << UI::FileInput([this](emp::File file){
      file.RemoveIfBegins("#");  // Remove all lines that are comments
      file.RemoveEmpty();
      ClearTable();

      for (emp::String line : file) {
        bool ignore = line.PopIf('-');
        emp::String name = line.PopWord();  // First entry on a line is the token name.
        emp::String regex = line.Trim();    // Regex is remainder, minus start & end whitespace.
        AddTableRow(name, regex, ignore);
      }
      doc.Div("token_div").Redraw();

    }, "load_input").SetCSS("display", "none");

    token_div << UI::Button([this](){
      doc.FileInput("load_input").DoClick();
      GenerateLexer();
      UpdateSandbox();
    }, "Load Token Types", "load_but")
      .SetCSS(button_style)
      .SetTitle("Load previously saved token types from file.");

    token_div << UI::Button{[this](){
      if (SwapTableRows(active_token, active_token+1)) {
        ActivateTableRow(active_token+1);
      }
      doc.Div("token_div").Redraw();
    }, "Row &darr;"}
      .SetCSS(button_style).SetCSS("float", "right", "padding", "5px 10px")
      .SetTitle("Move active row DOWN.");

    token_div << UI::Button{[this](){
      if (SwapTableRows(active_token, active_token-1)) {
        ActivateTableRow(active_token-1);
      }
      doc.Div("token_div").Redraw();
    }, "Row &uarr;"}
      .SetCSS(button_style).SetCSS("float", "right", "padding", "5px 10px")
      .SetTitle("Move active row UP.");
    


    token_div << "<br>";

    token_div << UI::Button([this](){
      GenerateCPP();
    }, "Generate C++ Code", "generate_but").SetCSS(button_style).SetBackground("#330066")
    .SetTitle("Generate a lexer using the token types defined above.");

    token_div << UI::Button([this](){
      ToggleSandbox();
      GenerateLexer();
      UpdateSandbox();
    }, "Open Sandbox", "sandbox_but").SetCSS(button_style).SetBackground("#330066")
    .SetTitle("Try out the current set of tokens live");

    token_div << UI::Button([this](){
      doc.Div("settings_div").ToggleActive();
    }, "Advanced Options", "settings_but")
      .SetCSS(button_style).SetCSS("float", "right", "border-radius", "15px", "font-size", "12px")
      .SetTitle("Adjust naming details for generated code.");
  }

  void InitializeSettingsDiv() {
    settings_div.SetBackground("tan").SetCSS(div_style);
    settings_div << HeadingName("Advanced Options");

    size_t row_id = 0;
    settings_table[row_id][0] << "&nbsp;";
    settings_table[row_id][1].SetCSS("font-weight", "bold").SetBackground("tan") << "Generated Filename:";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      out_filename = str;
    }, "set_filename").SetText(out_filename).SetWidth(250)
      .SetTitle("Filename to use if you download the generated lexer.");
    ++row_id;

    settings_table[row_id][0].SetColSpan(3).SetColor("darkblue")
      << "<big><b>Token Data to Store</b></big>";
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Store lexemes?";
    settings_table[row_id][2] << UI::CheckBox([this](bool in){ use_token_lexemes = in; }, "checkbox_lexemes")
      .SetChecked(use_token_lexemes)
      .SetTitle("Should we store found lexemes as part of the generated Token class?");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Store line numbers?";
    settings_table[row_id][2] << UI::CheckBox([this](bool in){ use_token_line_num = in; }, "checkbox_line_nums")
      .SetChecked(use_token_line_num)
      .SetTitle("Should we store the line number where a token was found as part of the generated Token class?");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Store columns?";
    settings_table[row_id][2] << UI::CheckBox([this](bool in){ use_token_column = in; }, "checkbox_cols")
      .SetChecked(use_token_column)
      .SetTitle("Should we store the column where a token was found as part of the generated Token class?");
    ++row_id;


    settings_table[row_id][0].SetColSpan(3).SetColor("darkblue")
      << "<big><b>Names to use in the generated C++ code</b></big>";
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Include Guards: ";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      inc_guards = str;
    }, "set_includes").SetText(inc_guards).SetWidth(250)
      .SetTitle("Unique name of include guards at top and bottom of generated C++ file.");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Namespace: ";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      name_space = str;
    }, "set_namespace").SetText(name_space).SetWidth(250)
      .SetTitle("Namespace where generated classes should be placed.");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Lexer class Name: ";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      lexer_name = str;
    }, "set_lexer_class").SetText(lexer_name).SetWidth(250)
      .SetTitle("Identifier name to use for the generated C++ Lexer class.");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "Token class Name: ";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      token_name = str;
    }, "set_token_class").SetText(token_name).SetWidth(250)
      .SetTitle("Identifier name to use for the generated C++ Token class.");
    ++row_id;

    settings_table[row_id][1].SetCSS("font-weight", "bold") << "DFA class Name: ";
    settings_table[row_id][2] << UI::TextArea([this](const std::string & str) {
      dfa_name = str;
    }, "set_dfa_class").SetText(dfa_name).SetWidth(250)
      .SetTitle("Identifier name to use for the generated C++ DFA class.");
    ++row_id;

    settings_div << settings_table;
  }

  void InitializeSandboxDiv() {
    // sandbox_input.SetText("# This is some sample text.\n# Replace this with whatever you want to have tokenized.\n# Feel free to resize this box to your liking.");
    sandbox_input.SetText("# Sample text; replace with whatever you want to try tokenizing.\n"
                          "int countdown = 10;\n"
                          "while (countdown > 0) {\n"
                          "  print(countdown);\n"
                          "  countdown = countdown - 1;\n"
                          "}\n"
                          "print(\"Boom!\");\n");

    sandbox_div.SetBackground("black").SetColor("white").SetCSS(div_style);
    sandbox_div << UI::Button([this](){
      GenerateLexer();
      UpdateSandbox();
    }, "Refresh", "sandbox_refresh_but").SetCSS(sandbox_but_style);
    sandbox_div << UI::Button([this](){
      sandbox_show_token_info = !sandbox_show_token_info;
      if (sandbox_show_token_info) {
        doc.Button("sandbox_token_info_but").SetLabel("Token Info: ON");
        doc.Button("sandbox_types_but").SetBackground("#220022").SetDisabled(false);
        doc.Button("sandbox_lines_but").SetBackground("#220022").SetDisabled(false);
        doc.Button("sandbox_ignore_but").SetBackground("#220022").SetDisabled(false);
      } else {
        doc.Button("sandbox_token_info_but").SetLabel("Token Info: OFF");
        doc.Button("sandbox_types_but").SetBackground("#606060").SetDisabled(true);
        doc.Button("sandbox_lines_but").SetBackground("#606060").SetDisabled(true);
        doc.Button("sandbox_ignore_but").SetBackground("#606060").SetDisabled(true);
      }
      UpdateSandbox();
    }, "Token Info: OFF", "sandbox_token_info_but").SetCSS(sandbox_but_style);
    sandbox_div << UI::Button([this](){
      sandbox_show_types = !sandbox_show_types;
      if (sandbox_show_types) doc.Button("sandbox_types_but").SetLabel("Types: ON");
      else doc.Button("sandbox_types_but").SetLabel("Types: OFF");
      UpdateSandbox();
    }, "Types: OFF", "sandbox_types_but").SetCSS(sandbox_but_style).SetBackground("#606060").SetDisabled(true);
    sandbox_div << UI::Button([this](){
      sandbox_show_lines = !sandbox_show_lines;
      if (sandbox_show_lines) doc.Button("sandbox_lines_but").SetLabel("Line Nums: ON");
      else doc.Button("sandbox_lines_but").SetLabel("Line Nums: OFF");
      UpdateSandbox();
    }, "Line Nums: OFF", "sandbox_lines_but").SetCSS(sandbox_but_style).SetBackground("#606060").SetDisabled(true);
    sandbox_div << UI::Button([this](){
      sandbox_show_ignore = !sandbox_show_ignore;
      if (sandbox_show_ignore) doc.Button("sandbox_ignore_but").SetLabel("Ignored: VISIBLE");
      else doc.Button("sandbox_ignore_but").SetLabel("Ignored: HIDDEN");
      GenerateLexer();
      UpdateSandbox();
    }, "Ignored: HIDDEN", "sandbox_ignore_but").SetCSS(sandbox_but_style).SetBackground("#606060").SetDisabled(true);
    sandbox_div << sandbox_input.SetSize(750, 115);
    sandbox_div << "<p>";
    sandbox_div << sandbox_text.SetWidth(750).SetBackground("black").SetColor("white");
    sandbox_div << "</p>";

    sandbox_input.SetCallback([this](std::string){ UpdateSandbox(); });

    sandbox_colors.push_back("#8888FF"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#99FF99"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#FFFF88"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#FF88FF"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#88FFFF"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#f58231"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#ffe119"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#bfef45"); sandbox_bgs.push_back("black");
    //sandbox_colors.push_back("#42d4f4"); sandbox_bgs.push_back("black");// Too similar to #88FFFF
    sandbox_colors.push_back("#4363d8"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#911eb4"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#f032e6"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#fabed4"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#ffd8b1"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#aaffc3"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#dcbeff"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#3cb44b"); sandbox_bgs.push_back("black");
    sandbox_colors.push_back("#8888FF"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#88FF88"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#FFFF88"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#FF88FF"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#88FFFF"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#f58231"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#ffe119"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#bfef45"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#3cb44b"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#42d4f4"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#4363d8"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#911eb4"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#f032e6"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#fabed4"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#ffd8b1"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#aaffc3"); sandbox_bgs.push_back("#404040");
    sandbox_colors.push_back("#dcbeff"); sandbox_bgs.push_back("#404040");
  }

  void UpdateSandbox() {
    if (sandbox_div.IsInactive() || !TestValidTable()) return;

    emp::TokenStream tokens("Emplex Sandbox");
    if (lexer.GetNumTokens()) {
      tokens = lexer.Tokenize(sandbox_input.GetText(), "Emplex Sandbox",
                              sandbox_show_ignore || !sandbox_show_token_info);
    }

    // EM_ASM({ alert(UTF8ToString($0)); }, emp::MakeString("# tokens = ", tokens.size()).c_str());

    sandbox_text.Freeze();
    sandbox_text.Clear();
    if (tokens.size() == 0) {
      sandbox_text << "NO VISIBLE TOKENS.";
    }
    for (auto token : tokens) {
      // Print information about the next token, if needed.
      if (sandbox_show_token_info) {
        sandbox_text << "[";
        if (sandbox_show_types) {
          const emp::String token_type_name = lexer.GetTokenName(token.id);
          sandbox_text << token_type_name << ":";
        }
        if (sandbox_show_lines) {
          sandbox_text << token.line_id << ":";
        }
      }

      // Determine the color of the next token.
      if (token.id == -1) {
        sandbox_text << "<span style=\"background-color:#440000; color:#FFCCCC\">";
      } else {
        size_t color_id = (255 - token.id) % sandbox_colors.size();
        sandbox_text << "<span style=\"color:" << sandbox_colors[color_id]
                     << "; background-color:" << sandbox_bgs[color_id] << "\">";
      }

      if (sandbox_show_token_info) {
        sandbox_text << token.lexeme.AsEscaped(false).AsWebSafe(true) << "</span>]";
      } else {
        sandbox_text << token.lexeme.AsWebSafe(true) << "</span>";
      }

    }
    sandbox_text.Activate();
  }

  void InitializeOutputDiv() {
    UI::Style icon_style{
      "background-color", "black",
      "position", "absolute",
      "cursor", "pointer",
      "top", "10px",
      "right", "10px",
      "border", "none"      
   };

    output_div.SetCSS("width", "830px", "position", "relative");
    output_div.SetBackground("black").SetColor("white");
    output_div.SetBorder("20px").SetCSS("border-radius", "10px");
    output_div << UI::Button([this](){ DownloadCode(); },
      "<img src=\"Icons/ICON-Save.png\" width=\"40px\">", "download_icon")
      .SetTitle("Download Code").SetCSS(icon_style).SetCSS("right", "120px").Hide();
    output_div << UI::Button([this](){ CopyCode(); },
      "<img src=\"Icons/ICON-Copy.png\" width=\"50px\">", "copy_icon")
      .SetTitle("Copy Code to Clipboard").SetCSS(icon_style).SetCSS("right", "60px").Hide();
    output_div << UI::Button([this](){ ClearOutput(); },
      "<img src=\"Icons/ICON-Close.png\" width=\"40px\">", "close_icon")
      .SetTitle("Close Code").SetCSS(icon_style).SetCSS("right", "10px").Hide();
    output_div << output_text;
  }

  void InitializeFooterDiv() {
    footer_div.SetBackground("#000044").SetColor("white").SetCSS(div_style);
    footer_div <<
      "Emplex was developed by Dr. Charles Ofria at Michigan State University, 2024-2025. "
      "See \"About\" for more information.";
  }

public:
  Emplex() {
    InitializeButtonDiv();
    InitializeTokenDiv();
    InitializeSettingsDiv();
    error_div.SetBackground("white").SetColor("red");
    InitializeSandboxDiv();
    InitializeOutputDiv();
    InitializeFooterDiv();

    // Place all of the divs into the document.
    doc << "<h1>Emplex: A C++ Lexer Generator</h1>";
    doc << button_div;
    doc << intro_div;
    doc << token_div;
    doc << settings_div;
    doc << error_div;
    doc << sandbox_div;
    doc << output_div;
    doc << footer_div;

    UpdateIntro("home");
    settings_div.Deactivate();
    sandbox_div.Deactivate();
  }
};


Emplex emplex{};

int emp_main() {
}