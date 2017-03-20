//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2017.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  The TableRow widget, which behaves like the Table widget, but focuses on a single row.
//
//  DO NOT include directly.  All files begining with '_' are for internal use only.

class TableRow : public TableWidget {
public:
  TableRow(size_t r, size_t c, const std::string & in_id="") : TableWidget(r,c,in_id) { ; }
  TableRow(const TableWidget & in) : TableWidget(in) { ; }
  TableRow(const Widget & in) : TableWidget(in) { ; }
  TableRow(internal::TableInfo * in_info, size_t _row=0) : TableWidget(in_info, _row, 0) { ; }

  // Apply CSS to appropriate component based on current state.
  void DoCSS(const std::string & setting, const std::string & value) override {
    Info()->rows[cur_row].extras.style.Set(setting, value);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  void DoAttr(const std::string & setting, const std::string & value) override {
    Info()->rows[cur_row].extras.attr.Set(setting, value);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  void DoListen(const std::string & event_name, size_t fun_id) override {
    Info()->rows[cur_row].extras.listen.Set(event_name, fun_id);
    if (IsActive()) Info()->ReplaceHTML();   // @CAO only should replace cell's CSS
  }

  bool InStateRow() const { return true; }

  void Clear() { Info()->ClearRow(cur_row); }
  void ClearStyle() { Info()->rows[cur_row].extras.style.Clear(); }
  void ClearAttr() { Info()->rows[cur_row].extras.attr.Clear(); }
  void ClearListen() { Info()->rows[cur_row].extras.listen.Clear(); }
  void ClearExtras() { Info()->rows[cur_row].extras.Clear(); }
  void ClearChildren() { Info()->ClearRowChildren(cur_row); }
  void ClearCells() { Info()->ClearRow(cur_row); }

  std::string GetCSS(const std::string & setting) override {
    return Info()->rows[cur_row].extras.GetStyle(setting);
  }

};
