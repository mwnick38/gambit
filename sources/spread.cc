//
// FILE: spread.cc -- Defines a 3 dimensional spreadsheet/table control.
//
// $Id$
//

#include <stdio.h>
#include "wx.h"
#include "wx_mf.h"
#ifdef wx_msw
#include "wx_bbar.h"
#else
#include "wx_tbar.h"
#endif
#ifdef __BORLANDC__
#pragma hdr_stop
#endif // __BORLANDC__
#include    "wxmisc.h"
#include "gambit.h"
#include    "spread.h"
#include    "gmisc.h"

// Global GDI objects
wxPen       *grid_line_pen;
wxPen       *grid_border_pen;
wxPen       *s_selected_pen;
wxPen       *s_hilight_pen;
wxPen       *s_white_pen;
wxBrush *s_white_brush;
wxBrush *s_hilight_brush;



gOutput &operator<<(gOutput &op, const SpreadSheet3D &)
{
    return op;
}

gOutput &operator<<(gOutput &op, const SpreadSheet &s)
{
    s.Output(op);
    return op;
}

gOutput &operator<<(gOutput &op, const SpreadDataCell &c)
{
    op << c.value;
    return op;
}

//****************************************************************************
//*                              SPREAD SHEET DRAW SETTINGS                  *
//****************************************************************************
SpreadSheetDrawSettings::SpreadSheetDrawSettings(SpreadSheet3D *_parent, int cols)
    : col_width(cols)
{
    ref_cnt = 1;                                        
    parent = _parent;
    
    if (!LoadOptions())
    {
        row_height = DEFAULT_ROW_HEIGHT;
        default_col_width = DEFAULT_COL_WIDTH;
        col_dim_char = TRUE;
        labels = 0;
        vert_fit = TRUE;
        data_font = wxTheFontList->FindOrCreateFont(11, wxMODERN, wxNORMAL, wxNORMAL);
        label_font = wxTheFontList->FindOrCreateFont(12, wxMODERN, wxNORMAL, wxNORMAL);
        gtext = TRUE;
        num_prec = 3;
        SaveOptions();
    }
    
    for (int i = 1; i <= col_width.Length(); i++) col_width[i] = DEFAULT_COL_WIDTH;
    x_scroll = y_scroll = 0;
    scrolling = FALSE;
    x_start = -1;
    y_start = -1;
    ToTextPrecision(num_prec);
}


void    SpreadSheetDrawSettings::SetOptions(void)
{
    Bool labels_col = ColLabels(), labels_row = RowLabels();
    int horiz = col_width[1], vert = row_height;
    
    MyDialogBox *options_dialog = new MyDialogBox((wxWindow *)parent, "Options");
    options_dialog->Add(wxMakeFormMessage("Fonts"));
    wxFormItem *lfont_but = wxMakeFormButton("Label", (wxFunction)spread_options_lfont_func);
    options_dialog->Add(lfont_but);
    wxFormItem *dfont_but = wxMakeFormButton("Data", (wxFunction)spread_options_dfont_func);
    options_dialog->Add(dfont_but);
    options_dialog->Add(wxMakeFormNewLine());
    options_dialog->Add(wxMakeFormMessage("Cell size"));
    options_dialog->Add(wxMakeFormNewLine());
    options_dialog->Add(wxMakeFormShort("Horiz", &horiz, wxFORM_SLIDER, 
                                        new wxList(wxMakeConstraintRange(0, 50), 0)));
    options_dialog->Add(wxMakeFormBool("char", &horiz_fit));
    wxStringList *column_list = new wxStringList;
    char *col_str = new char[10];
    column_list->Add("None");
    column_list->Add("All");
    column_list = wxStringListInts(col_width.Length(), column_list);
    options_dialog->Add(wxMakeFormString("Col", &col_str, wxFORM_CHOICE,
                                         new wxList(wxMakeConstraintStrings(column_list), 0)));
    options_dialog->Add(wxMakeFormNewLine());
    options_dialog->Add(wxMakeFormShort("Vert", &vert, wxFORM_SLIDER, 
                                        new wxList(wxMakeConstraintRange(0, 50), 0)));
    options_dialog->Add(wxMakeFormBool("Fit to font", &vert_fit));
    options_dialog->Add(wxMakeFormNewLine());
    options_dialog->Add(wxMakeFormMessage("Show Labels"));
    options_dialog->Add(wxMakeFormBool("row", &labels_row));
    options_dialog->Add(wxMakeFormBool("col", &labels_col));
    options_dialog->Add(wxMakeFormBool("Color Text", &gtext));
    options_dialog->Add(wxMakeFormNewLine());
    options_dialog->Add(wxMakeFormShort("Output precision", &num_prec, wxFORM_SLIDER,
                                        new wxList(wxMakeConstraintRange(0, 16), 0)));
    options_dialog->Add(wxMakeFormNewLine());
    Bool save = FALSE;
    options_dialog->Form()->Add(wxMakeFormBool("Save now", &save));
    options_dialog->Form()->AssociatePanel(options_dialog);
    ((wxButton *)dfont_but->GetPanelItem())->SetClientData((char *)this);
    ((wxButton *)lfont_but->GetPanelItem())->SetClientData((char *)this);
    options_dialog->Go1();
    
    if (options_dialog->Completed() == wxOK)
    {
        unsigned int changed = 0; // what exactly has changed ...
        int which_col = wxListFindString(column_list, col_str);
        
        if (which_col) 
            SetColWidth(horiz, which_col-1);

        SetRowHeight(row_height);
        labels = 0;
        
        if (labels_row) 
            labels |= S_LABEL_ROW;
        
        if (labels_col) 
            labels |= S_LABEL_COL;
        
        if (ToTextPrecision() != num_prec)
        {
            ToTextPrecision(num_prec);
            changed |= S_PREC_CHANGED;
        }
        
        if (save) 
            SaveOptions();

        parent->OnOptionsChanged(changed);
    }

    delete options_dialog;
    delete col_str;
    parent->Redraw();
}


void    SpreadSheetDrawSettings::spread_options_lfont_func(wxButton &ob, wxEvent &)
{
    SpreadSheetDrawSettings  *draw_settings = (SpreadSheetDrawSettings *)ob.GetClientData();
    FontDialogBox *f = new FontDialogBox(NULL, draw_settings->GetLabelFont());
    
    if (f->Completed() == wxOK)
        draw_settings->SetLabelFont(f->MakeFont());

    delete f;
}


void    SpreadSheetDrawSettings::spread_options_dfont_func(wxButton &ob, wxEvent &)
{
    SpreadSheetDrawSettings *draw_settings = (SpreadSheetDrawSettings *)ob.GetClientData();
    FontDialogBox *f = new FontDialogBox(NULL, draw_settings->GetDataFont());
    
    if (f->Completed() == wxOK)
        draw_settings->SetDataFont(f->MakeFont());

    delete f;
}


void    SpreadSheetDrawSettings::SaveOptions(const char *s)
{
    char *file_name;
    const char *sn = "SpreadSheet3D";   // section name
    file_name = copystring((s) ? s : (char *) gambitApp.ResourceFile());
    
    wxWriteResource(sn, "SpreadSheet3D-Version", 2, file_name);
    wxWriteResource(sn, "Row-Height", row_height, file_name);
    wxWriteResource(sn, "Default-Column-Width", default_col_width, file_name);
    wxWriteResource(sn, "Col-Dim-Char", col_dim_char, file_name);
    wxWriteResource(sn, "Fit-Text-Vert", vert_fit, file_name);
    wxWriteResource(sn, "Fit-Text-Horiz", horiz_fit, file_name);
    wxWriteResource(sn, "Show-Labels", show_labels, file_name);
    wxWriteResource(sn, "Data-Font", wxFontToString(data_font), file_name);
    wxWriteResource(sn, "Label-Font", wxFontToString(label_font), file_name);
    wxWriteResource(sn, "Use-GText", gtext, file_name);
    wxWriteResource("Gambit", "Output-Precision", num_prec, file_name);
    delete [] file_name;
}


int SpreadSheetDrawSettings::LoadOptions(const char *s)
{
    const char *sn = "SpreadSheet3D";   // section name
    const char *file_name = (s) ? s : (char *) gambitApp.ResourceFile();
    
    char *font_str = new char[100];
    int version = 0;
    wxGetResource(sn, "SpreadSheet3D-Version", &version, file_name);
    
    if (!version) 
        return 0;

    wxGetResource(sn, "Row-Height", &row_height, file_name);
    wxGetResource(sn, "Default-Column-Width", &default_col_width, file_name);
    wxGetResource(sn, "Col-Dim-Char", &col_dim_char, file_name);
    wxGetResource(sn, "Fit-Text-Vert", &vert_fit, file_name);
    wxGetResource(sn, "Fit-Text-Horiz", &horiz_fit, file_name);
    wxGetResource(sn, "Show-Labels", &show_labels, file_name);
    wxGetResource(sn, "Data-Font", &font_str, file_name);
    data_font = wxStringToFont(font_str);
    wxGetResource(sn, "Label-Font", &font_str, file_name);
    label_font = wxStringToFont(font_str);
    wxGetResource(sn, "Use-GText", &gtext, file_name);
    wxGetResource("Gambit", "Output-Precision", &num_prec, file_name);
    
    return 1;
}


// Column width
int SpreadSheetDrawSettings::GetColWidth(int col)
{
    if (!col) 
        col = 1;
    else 
        assert(col >= 1 && col <= col_width.Length() && "ColWidth::Invalid");
    
    if (horiz_fit)
        return col_width[col]*tw+2*TEXT_OFF;
    else
        return col_width[col]*COL_WIDTH_UNIT;
}


void SpreadSheetDrawSettings::SetColWidth(int _c, int col)
{
    if (col) 
    {
        col_width[col] = _c;
    }
    else
    {
        for (int i = 1; i <= col_width.Length(); i++) 
            col_width[i] = _c;
    }
}


// Font Size.  If the cell size is tied to the font size, i.e. if
// vert_fit is used, or if the cell width is measured in chars, we
// need to update these values every time the font changes
void SpreadSheetDrawSettings::UpdateFontSize(float x, float y)
{
    if (x < 0 && y < 0)
    {
        parent->GetDataExtent(&x, &y, "A quick greW");
        x /= 12;
    }

    tw = (int)x;
    th = (int)y;
}


// We need to know how many columns there are ...
void SpreadSheetDrawSettings::SetDimensions(int /*rows*/, int cols)
{
    int old_cols = col_width.Length();

    if (cols > old_cols)
    {
        for (int i = 1; i <= cols-old_cols; i++) 
            AddCol();
    }

    if (cols < col_width.Length())
    {
        for (int i = old_cols; i <= cols; i--) 
            col_width.Remove(i);
    }
}



//****************************************************************************
//*                             SPREAD SHEET DATA SETTINGS                   *
//****************************************************************************
void SpreadSheetDataSettings::SetAutoLabelStr(const gText s, int what)
{
    switch (what)
    {
    case S_AUTO_LABEL_ROW:  
        auto_label_row = s;
        break;

    case S_AUTO_LABEL_COL:  
        auto_label_col = s;
        break;

    case S_AUTO_LABEL_LEVEL:    
        auto_label_level = s;
        break;
    }
}


gText   SpreadSheetDataSettings::AutoLabelStr(int what) const
{
    gText label;
    switch (what)
    {
    case S_AUTO_LABEL_ROW:  
        label = auto_label_row;
        break;

    case S_AUTO_LABEL_COL:  
        label = auto_label_col;
        break;

    case S_AUTO_LABEL_LEVEL:    
        label = auto_label_level;
        break;

    default: 
        label = "";
        break;
    }

    return label;
}

//****************************************************************************
//*                               SPREAD SHEET CANVAS                        *
//****************************************************************************

// Constructor
SpreadSheetC::SpreadSheetC(SpreadSheet *_sheet, wxFrame *parent, 
                           int x, int y, int w, int h)
    : wxCanvas(parent, x, y, w, h, 0)
{
    top_frame = (SpreadSheet3D *)parent;
    sheet = _sheet;
    draw_settings = top_frame->DrawSettings();
    data_settings = top_frame->DataSettings();

    // Make sure that draw_settings knows about the current font size
    float tw, th;
    GetDataExtent(&tw, &th, "A quick greW");
    draw_settings->UpdateFontSize(tw/12, th);

    // Allow double clicking on canvas
    AllowDoubleClick(TRUE);

    // Give myself some scrollbars if necessary
    //CheckScrollbars();
    Show(FALSE);    // Do not update myself until told by the parent
}


int SpreadSheetC::MaxX(int col)
{
    int temp = draw_settings->XStart();
    
    if (col < 0) 
        col = sheet->GetCols();

    for (int i = 1; i <= col; i++)  
        temp += draw_settings->GetColWidth(i);

    return temp;
}


int SpreadSheetC::MaxY(int row)
{
    int temp = draw_settings->YStart();
    
    if (row < 0) 
        row = sheet->GetRows();

    temp += draw_settings->GetRowHeight() * row;

    return temp;
}


Bool SpreadSheetC::XYtoRowCol(int x, int y, int *row, int *col)
{
  if (x < MaxX(0) || y < MaxY(0) || x > MaxX() || y > MaxY()) {
    return FALSE;
  }

  *row = (y-draw_settings->YStart()) / draw_settings->GetRowHeight()+1;
  *col = 0;
  int i = 1;

  while (*col == 0 && i <= sheet->GetCols())
    {
        if (x < MaxX(i)) 
            *col = i;

        i++;
    }
    
    if (*row < 1) 
        *row = 1;
    
    if (*row > sheet->GetRows()) 
        *row = sheet->GetRows();
    
    if (*col < 1) 
        *col = 1;
    
    if (*col > sheet->GetCols()) 
        *col = sheet->GetCols();

    return TRUE;
}


void SpreadSheetC::OnSize(int _w, int _h)
{
    int h, w;
    GetVirtualSize(&w, &h);
    
    if (w == 0) 
        draw_settings->SetWidth(_w);
    else 
        draw_settings->SetWidth(w);
    
    if (h == 0) 
        draw_settings->SetHeight(_h);
    else 
        draw_settings->SetHeight(h);

    draw_settings->SetRealHeight(_h);
    draw_settings->SetRealWidth(_w);
}


// Desired Size
void    SpreadSheetC::DesiredSize(int *w, int *h)
{
    *w = gmin(MaxX(), MAX_SHEET_WIDTH);
    *h = gmin(draw_settings->YStart() +
              (sheet->GetRows()+1) * draw_settings->GetRowHeight()+3, 
              MAX_SHEET_HEIGHT);
    *w = gmax(*w, MIN_SHEET_WIDTH);
    *h = gmax(*h, MIN_SHEET_HEIGHT);
}


// Check Scrollbars
void    SpreadSheetC::CheckScrollbars(void)
{
    int x_step = -1, y_step = -1;
    
    if (MaxX() > MAX_SHEET_WIDTH)
        x_step = MaxX()/XSTEPS+5;
    
    if (draw_settings->YStart() + sheet->GetRows() * draw_settings->GetRowHeight() > 
        MAX_SHEET_HEIGHT)
        y_step = (draw_settings->YStart() + (sheet->GetRows() + 1) *
                  draw_settings->GetRowHeight()) / YSTEPS + 5;
    
    if (x_step > 0 || y_step > 0)
    {
        // Note that due to a bug in SetClientSize, we must either have no or both scrollbars
        if (x_step <= 0) 
            x_step = MaxX()/XSTEPS+5;
        
        if (y_step <= 0) 
            y_step = (draw_settings->YStart()+(sheet->GetRows()+1) *
                      draw_settings->GetRowHeight())/YSTEPS+5;
        
        if (x_step != draw_settings->XScroll() || y_step != draw_settings->YScroll())
        {
            ((wxCanvas *)this)->SetScrollbars(x_step, y_step, XSTEPS, YSTEPS, 4, 4);
            draw_settings->SetXScroll(x_step);
            draw_settings->SetYScroll(y_step);
            draw_settings->SetScrolling(TRUE);
        }
    }
    
    if (x_step < 0 && y_step < 0 && draw_settings->Scrolling())
    {
        x_step = 1;
        y_step = 1;
        SetScrollbars(x_step, y_step, XSTEPS, YSTEPS, 4, 4);
        draw_settings->SetXScroll(x_step);
        draw_settings->SetYScroll(y_step);
        draw_settings->SetScrolling(FALSE);
    }
    
}


// Paint message handler
void SpreadSheetC::OnPaint(void)
{
    Update(*(GetDC()));
}

// Mouse message handler
void SpreadSheetC::OnEvent(wxMouseEvent &ev)
{
    if (top_frame->OnEventNew(ev)) 
        return;
    
    if (ev.LeftDown() || ev.ButtonDClick())
    {
        float x, y;
        ev.Position(&x, &y);
        
        if (sheet->XYtoRowCol(x, y, &cell.row, &cell.col) == FALSE) 
            return;
        
        if (ev.LeftDown() && !ev.ControlDown()) 
            ProcessCursor(0);
        
        if (ev.ButtonDClick() || (ev.LeftDown() && ev.ControlDown()))
            top_frame->OnDoubleClick(cell.row, cell.col, 
                                     sheet->GetLevel(), 
                                     sheet->GetValue(cell.row, cell.col));
	top_frame->CanvasFocus();
    }
}


// Keyboard message handler
void SpreadSheetC::OnChar(wxKeyEvent &ev)
{
    // Allow the default behavior to be overriden.
    if (top_frame->OnCharNew(ev)) 
        return;

    int ch = ev.KeyCode();

    // Ignore shift key.
    // Note: this will not affect the use of the shift key e.g. in
    // changing the case of a character.
    if (ch == WXK_SHIFT)
        return;

    // Cursor keys to move the highlight.
    if (IsCursor(ev) || IsEnter(ev))
    {
        ProcessCursor(ch);
        return;
    }

    // F2 on the last row adds a row.
    if (ch == WXK_F2)
    {
        if (data_settings->Change(S_CAN_GROW_ROW) && cell.row == sheet->GetRows()) 
            top_frame->AddRow();

        return;
    }

    // F3 on the last column adds a column.
    if (ch == WXK_F3)
    {
        if (data_settings->Change(S_CAN_GROW_COL) && cell.col == sheet->GetCols()) 
            top_frame->AddCol();

        return;
    }

    // Otherwise, if editing is enabled, just process the key.
    {
        if (top_frame->Editable())
        {
            gSpreadValType cell_type = sheet->GetType(cell.row, cell.col);
            
            if ((cell_type == gSpreadNum && IsNumeric(ev)) ||   
                (cell_type == gSpreadStr && IsAlphaNum(ev)))
            {
                if (cell.editing == FALSE)
                {
                    cell.editing = TRUE;

                    // Preserve the previously-existing color of the cell if any.
                    if (cell.str.Left(3) == "\\C{")
                    {
                        gText prefix = "\\C{";
                        // Append the color number.
                        for (int k = 3; k < cell.str.Length(); k++)
                        {
                            if (cell.str[k] != '}')
                            {
                                prefix += cell.str[k];
                            }
                            else // We found the color delimiter "}".
                            {
                                prefix += "}";
                                break;
                            }
                        }

                        cell.str = prefix;
                    }
                    else
                    {
                        cell.str = "";
                    }
                } // this implements 'overwrite'

                cell.str += (char) ch;
            }
            
            if (IsDelete(ev))
            {
                if (cell.editing == FALSE) 
                    cell.editing = TRUE;
                
                if (cell.str.Length()) 
                    cell.str.Remove(cell.str.Length()-1);
            }

            top_frame->SetStatusText(cell.str);

            // Update the character in place.
            sheet->SetValue(cell.row, cell.col, cell.str);
            UpdateCell(*(GetDC()), cell);
        }
    }
}


void SpreadSheetC::ProcessCursor(int ch)
{
    if (cell.editing && ch != 0) 
        sheet->SetValue(cell.row, cell.col, cell.str);

    SpreadMoveDir how = SpreadMoveJump;

    switch (ch)
    {
    case    WXK_UP:
        if (cell.row > 1) {
            if (!sheet->GetSelectableRow(cell.row - 1))  return;
            cell.row--;
            how = SpreadMoveUp;
        }
        break;

    case    WXK_DOWN:
        if (cell.row < sheet->GetRows()) {
            if (!sheet->GetSelectableRow(cell.row + 1))  return;
            cell.row++;
            how = SpreadMoveDown;
        }
        break;

    case    WXK_RIGHT:
        if (cell.col < sheet->GetCols()) {
            if (!sheet->GetSelectableCol(cell.col + 1))  return;
            cell.col++;
            how = SpreadMoveRight;
        }
        break;

    case WXK_LEFT:
        if (cell.col > 1) {
            if (!sheet->GetSelectableCol(cell.col - 1))  return;
            cell.col--;
            how = SpreadMoveLeft;
        }
        break;

    default:
        break;
    }

    cell.Reset(sheet->GetValue(cell.row, cell.col));
    UpdateCell(*(GetDC()), cell);

    // MCV: commented this out; it shouldn't do anything anyway
    // but for some reason if it's uncommented it makes the
    // spreadsheet do weird things when you select the 
    // bottommost cell after scrolling (it jumps the view
    // back to the top and adds an extra row to the SS).

    top_frame->OnSelectedMoved(cell.row, cell.col, how);
    top_frame->SetStatusText(gPlainText(cell.str));

    // Make sure the cursor is visible by adjusting the scrollbar
    // position.  Don't adjust the scrollbars if this was a mouse
    // movement (ch = 0).
    if (draw_settings->Scrolling() && ch != 0)
    {
		Bool rescroll = FALSE;

        int cx, cy;  // Current x and y scroll positions.
        ViewStart(&cx, &cy);
        int x_scroll = cx;
		int y_scroll = cy;

		int cell_x_min = MaxX(cell.col - 1);  // Minimum x value of cell.
		int cell_x_max = MaxX(cell.col);      // Maximum x value of cell.
		int cell_y_min = MaxY(cell.row - 1);  // Minimum y value of cell.
		int cell_y_max = MaxY(cell.row);      // Maximum y value of cell.
        int x_scroll_width = draw_settings->XScroll();
		int y_scroll_width = draw_settings->YScroll();
		int window_x_min = x_scroll_width * cx;
		int window_x_max = window_x_min + draw_settings->GetRealWidth();
		int window_y_min = y_scroll_width * cy;
		int window_y_max = window_y_min + draw_settings->GetRealHeight();

		// Leave the scrollbars where they are if the new cell position
		// will fit within them.  Otherwise, position the scrollbar(s)
		// so that the new position of the cell is as far left (for 
		// movement in the X direction) or as far up (for movement in
		// the Y direction) as possible.

		if ((cell_x_min < window_x_min) || (cell_x_max >= window_x_max))
		{
			x_scroll = cell_x_min / x_scroll_width;
			rescroll = TRUE;
		}

		if ((cell_y_min < window_y_min) || (cell_y_max >= window_y_max))
		{
			y_scroll = cell_y_min / y_scroll_width;
			rescroll = TRUE;
		}

        if (rescroll)
        {
            Scroll(x_scroll, y_scroll);
        }
    }
    top_frame->CanvasFocus();
}


void SpreadSheetC::UpdateCell(wxDC &dc, SpreadCell &cell)
{
    // Check for the validity of the cell/old_cell
    cell.CheckValid(sheet->GetRows(), sheet->GetCols());
    old_cell.CheckValid(sheet->GetRows(), sheet->GetCols());

    // erase the old hilight
    DrawCell(dc, old_cell.row, old_cell.col);

    // draw the new hilight
    dc.SetBrush(wxTRANSPARENT_BRUSH);
    dc.SetPen(s_selected_pen);
    dc.DrawRectangle(MaxX(cell.col-1)+LINE_OFF,
                     draw_settings->YStart()+(cell.row-1)*draw_settings->GetRowHeight()+LINE_OFF,
                     draw_settings->GetColWidth(cell.col)-2*LINE_OFF,
                     draw_settings->GetRowHeight()-2*LINE_OFF);

    // Update the status line text on the topmost frame
    top_frame->SetStatusText(gPlainText(cell.str));

    // Save the new cell
    old_cell = cell;
}


// Draw a cell
void SpreadSheetC::DrawCell(wxDC &dc, int row, int col)
{
    dc.SetFont(draw_settings->GetDataFont());
    
    if (sheet->HiLighted(row, col))
    {
        dc.SetBrush(s_hilight_brush);
        dc.SetPen(s_hilight_pen);
    }
    else
    {
        dc.SetBrush(s_white_brush);
        dc.SetPen(s_white_pen);
    }
    
    if (sheet->Bold(row, col))
    {
        wxFont *cur = draw_settings->GetDataFont();
        dc.SetFont(wxTheFontList->FindOrCreateFont(cur->GetPointSize(), 
                                                   cur->GetFamily(),
                                                   cur->GetStyle(), 
                                                   wxBOLD, 
                                                   cur->GetUnderlined()));
    }

    dc.DrawRectangle(MaxX(col-1)+LINE_OFF,
                     draw_settings->YStart()+(row-1)*draw_settings->GetRowHeight()+LINE_OFF,
                     draw_settings->GetColWidth(col)-2*LINE_OFF,
                     draw_settings->GetRowHeight()-2*LINE_OFF);

    dc.SetClippingRegion(MaxX(col-1)+TEXT_OFF,
                         draw_settings->YStart()+(row-1)*draw_settings->GetRowHeight()+TEXT_OFF,
                         draw_settings->GetColWidth(col)-2*TEXT_OFF,
                         draw_settings->GetRowHeight()-2*TEXT_OFF);
    
    if (draw_settings->UseGText())  // use possibly colored text
    {
        gDrawText(dc, sheet->GetValue(row, col),
                  MaxX(col-1)+TEXT_OFF,
                  draw_settings->YStart()+(row-1)*draw_settings->GetRowHeight()+
                  draw_settings->GetTextHeight()/4+TEXT_OFF);
    }
    else
    {
        gDrawText(dc, gPlainText(sheet->GetValue(row, col)),
                  MaxX(col-1)+TEXT_OFF,
                  draw_settings->YStart()+(row-1)*draw_settings->GetRowHeight()+TEXT_OFF);
    }
    
    dc.DestroyClippingRegion();
}


// Updating
// Changed the code to only redraw the visible part of the window.  This
// should make updating large spreadsheets considerably faster.
void SpreadSheetC::Update(wxDC &dc)
{
    int row, col;

    // Find the visible dimensions
    int min_row, max_row, min_col, max_col;
    int x_start, y_start;
    int width = draw_settings->GetRealWidth(), height = draw_settings->GetRealHeight();
    
    if (!height || !width) 
        return;

    ViewStart(&x_start, &y_start);
    x_start *= draw_settings->XScroll();
    y_start *= draw_settings->YScroll();
    min_row = (y_start-draw_settings->YStart())/draw_settings->GetRowHeight()+1;

    if (min_row < 1) 
        min_row = 1;

    max_row = min_row+height/draw_settings->GetRowHeight();
    
    if (max_row > sheet->GetRows()) 
        max_row = sheet->GetRows();

    min_col = 0;
    int i = 1;

    while (!min_col && i <= sheet->GetCols())
    {
        if (x_start < MaxX(i)) 
            min_col = i;

        i++;
    }
    
    if (min_col < 1) 
        min_col = 1;

    max_col = 0;
    i = 1;

    while (!max_col && i <= sheet->GetCols())
    {
        if (x_start+width < MaxX(i)) 
            max_col = i;

        i++;
    }
    
    if (max_col < 1) 
        max_col = sheet->GetCols();

    // Draw the grid
    char *dc_type = dc.GetClassInfo()->GetClassName();
    
    if (strcmp(dc_type, "wxMetaFileDC") != 0)
    {
        dc.Clear();
        dc.BeginDrawing();
    }
    
    if (strcmp(dc_type, "wxPostScriptDC") != 0) 
        dc.SetBackgroundMode(wxTRANSPARENT);

    dc.SetBrush(wxTRANSPARENT_BRUSH);
    dc.SetPen(grid_line_pen);

    for (row = min_row; row <= max_row; row++)
    {
        dc.DrawLine(draw_settings->XStart(), 
                    draw_settings->YStart()+row*draw_settings->GetRowHeight(),
                    MaxX(max_col)+1,
                    draw_settings->YStart()+row*draw_settings->GetRowHeight());
    }

    for (col = min_col; col <= max_col; col++)
    {
        dc.DrawLine(MaxX(col), 
                    draw_settings->YStart(),
                    MaxX(col),
                    draw_settings->YStart()+max_row*draw_settings->GetRowHeight()+1);
    }

    dc.SetPen(grid_border_pen);
    dc.SetBrush(wxTRANSPARENT_BRUSH); // draw bounding rectangle out of 4 lines
    
    if (min_col == 1)   // left
    {
        dc.DrawLine(draw_settings->XStart(), 
                    draw_settings->YStart(), 
                    draw_settings->XStart(),
                    max_row*draw_settings->GetRowHeight()+2+draw_settings->YStart());
    }
    
    if (max_col == sheet->GetCols()) // right
    {
        dc.DrawLine(MaxX(), 
                    draw_settings->YStart(), 
                    MaxX(),
                    max_row*draw_settings->GetRowHeight()+2+draw_settings->YStart());
    }
    
    if (min_row == 1) // top
    {
        dc.DrawLine(draw_settings->XStart(), 
                    draw_settings->YStart(), 
                    MaxX(max_col), 
                    draw_settings->YStart());
    }
    
    if (max_row == sheet->GetRows()) // bottom
    {
        dc.DrawLine(draw_settings->XStart(), 
                    max_row*draw_settings->GetRowHeight()+2+draw_settings->YStart(),
                    MaxX(), 
                    max_row*draw_settings->GetRowHeight()+2+draw_settings->YStart());
    }

    // Draw the labels if any (no sense in showing them if even the first row/col
    // are not visible
    dc.SetFont(draw_settings->GetLabelFont());
    
    if (draw_settings->RowLabels() && min_col == 1)
    {
        for (row = min_row; row <= max_row; row++)
            dc.DrawText(sheet->GetLabelRow(row), 0, 
                        draw_settings->YStart() + 
                        (row-1)*draw_settings->GetRowHeight()+TEXT_OFF);
    }
    
    if (draw_settings->ColLabels() && min_row == 1)
    {
        for (col = min_col; col <= max_col; col++)
            dc.DrawText(sheet->GetLabelCol(col), MaxX(col-1)+TEXT_OFF, 0);
    }

    // Fill in the cells
    for (row = min_row; row <= max_row; row++)
        for (col = min_col; col <= max_col; col++)
            DrawCell(dc, row, col);
    
    // Hilight the currently selected cell
    UpdateCell(dc, cell);
    
    if (strcmp(dc_type, "wxMetaFileDC") != 0)
        dc.EndDrawing();
}


//****************************************************************************
//*                               SPREAD SHEET PRINTOUT
//****************************************************************************
#ifdef wx_msw
#include "wx_print.h"

class SpreadSheetPrintout: public wxPrintout
{
private:
    SpreadSheetC *sheet;
    wxOutputOption fit;
    int num_pages;

public:
    SpreadSheetPrintout(SpreadSheetC *s, wxOutputOption f, 
                        const char *title = "SpreadPrintout");
    Bool OnPrintPage(int page);
    Bool HasPage(int page);
    Bool OnBeginDocument(int startPage, int endPage);
    void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo);
};


SpreadSheetPrintout::SpreadSheetPrintout(SpreadSheetC *s, 
                                         wxOutputOption f, const char *title)
    : sheet(s), fit(f), wxPrintout((char *)title)
{ }

Bool SpreadSheetPrintout::OnBeginDocument(int startPage, int endPage)
{
    if (!wxPrintout::OnBeginDocument(startPage, endPage))
        return FALSE;
    
    return TRUE;
}

// Since we can not get at the actual device context in this function, we
// have no way to tell how many pages will be used in the wysiwyg mode. So,
// we have no choice but to disable the From:To page selection mechanism.
void SpreadSheetPrintout::GetPageInfo(int *minPage, int *maxPage, 
                                      int *selPageFrom, int *selPageTo)
{
    num_pages = 1;
    *minPage = 0;
    *maxPage = num_pages;
    *selPageFrom = 0;
    *selPageTo = 0;
}


Bool SpreadSheetPrintout::HasPage(int pageNum)
{
    return (pageNum <= num_pages);
}


Bool SpreadSheetPrintout::OnPrintPage(int /*page*/)
{
    wxDC *dc = GetDC();
    
    if (!dc) 
        return FALSE;
    
    // Get the logical pixels per inch of screen and printer
    int ppiScreenX, ppiScreenY;
    GetPPIScreen(&ppiScreenX, &ppiScreenY);
    int ppiPrinterX, ppiPrinterY;
    GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
    
    // Now we have to check in case our real page size is reduced
    // (e.g. because we're drawing to a print preview memory DC)
    int pageWidth, pageHeight;
    float w, h;
    dc->GetSize(&w, &h);
    GetPageSizePixels(&pageWidth, &pageHeight);
    float pageScaleX = (float)w/pageWidth;
    float pageScaleY = (float)h/pageHeight;
    dc->SetBackgroundMode(wxTRANSPARENT);
    
    if (!fit) // WYSIWYG
    {
        // This scales the DC so that the printout roughly represents the
        // the screen scaling. The text point size _should_ be the right size
        // but in fact is too small for some reason. This is a detail that will
        // need to be addressed at some point but can be fudged for the
        // moment.
        float scaleX = (float)((float)ppiPrinterX/(float)ppiScreenX);
        float scaleY = (float)((float)ppiPrinterY/(float)ppiScreenY);
        
        // If printer pageWidth == current DC width, then this doesn't
        // change. But w might be the preview bitmap width, so scale down.
        float overallScaleX = scaleX * (float)pageScaleX;
        float overallScaleY = scaleY * (float)pageScaleY;
        dc->SetUserScale(overallScaleX, overallScaleY);
        
        // Make the margins.  They are just 1" on all sides now.
        float marginX = 1*ppiPrinterX, marginY = 1*ppiPrinterY;
        dc->SetDeviceOrigin(marginX*pageScaleX, marginY*pageScaleY);
        
        sheet->draw_settings->SetRealWidth((pageWidth-2*marginX)/scaleX);
        sheet->draw_settings->SetRealHeight((pageHeight-2*marginY)/scaleY);
    }
    else    // FIT TO PAGE
    {
        float maxX = sheet->MaxX();
        float maxY = sheet->MaxY();
        
        // Make the margins.  They are just 1" on all sides now.
        float marginX = 1*ppiPrinterX;
        float marginY = 1*ppiPrinterY;
        
        // Calculate a suitable scaling factor
        float scaleX = (float)((pageWidth-2*marginX)/maxX)*pageScaleX;
        float scaleY = (float)((pageHeight-2*marginY)/maxY)*pageScaleY;
        
        // Use x or y scaling factor, whichever fits on the DC
        float actualScale = gmin(scaleX, scaleY);
        
        // Set the scale and origin
        dc->SetUserScale(actualScale, actualScale);
        dc->SetDeviceOrigin(marginX*pageScaleX, marginY*pageScaleY);
        
        // Let the spreadsheet know the new dimensions
        sheet->draw_settings->SetRealWidth(maxX);
        sheet->draw_settings->SetRealHeight(maxY);
        sheet->Scroll(0, 0);    // bad--should be set in draw_settings.
    }
    
    sheet->Update(*dc);
    
    return TRUE;
}
#endif


void SpreadSheetC::Print(wxOutputMedia device, wxOutputOption fit)
{
    if (device == wxMEDIA_PRINTER)
    {
#ifdef wx_msw
        wxPrinter printer;
        SpreadSheetPrintout printout(this, fit);
        printer.Print(top_frame, &printout, TRUE);
#else
        wxMessageBox("Printing not supported under X");
#endif
    }
    
    if (device == wxMEDIA_PREVIEW)
    {
#ifdef wx_msw
        wxPrintPreview *preview = 
            new wxPrintPreview(new SpreadSheetPrintout(this, fit), 
                               new SpreadSheetPrintout(this, fit));
        wxPreviewFrame *frame =
            new wxPreviewFrame(preview, top_frame, "Print Preview", 
                               100, 100, 600, 650);
        frame->Centre(wxBOTH);
        frame->Initialize();
        frame->Show(TRUE);
#else
        wxMessageBox("Previewing not supported under X");
#endif
    }
    
    if (device == wxMEDIA_CLIPBOARD || device == wxMEDIA_METAFILE)
    {
#ifdef wx_msw
        // Make the metafile just 640x480 and scale the sheet accordingly
        char *metafile_name = 0;
        
        if (device == wxMEDIA_METAFILE)
        {
            metafile_name = 
                copystring(wxFileSelector("Save Metafile", 0, 0, ".wmf", "*.wmf"));
        }

        wxMetaFileDC dc_mf(metafile_name);
        
        if (dc_mf.Ok())
        {
            float real_scale = gmin(640/MaxX(), 480/MaxY());
            dc_mf.SetUserScale(real_scale, real_scale);
            draw_settings->SetRealWidth(MaxX());
            draw_settings->SetRealHeight(MaxY());
            Update(dc_mf);
            wxMetaFile *mf = dc_mf.Close();
            
            if (mf)
            {
                Bool success = mf->SetClipboard(MaxX(), MaxY());
                
                if (!success) 
                    wxMessageBox("Copy Failed", "Error", wxOK | wxCENTRE, this);

                delete mf;
            }
            
            if (device == wxMEDIA_METAFILE) 
                wxMakeMetaFilePlaceable(metafile_name, 0, 0, MaxX(), MaxY());
        }
#else
        wxMessageBox("Metafiles not supported under X");
#endif
    }
    
    if (device == wxMEDIA_PS)
    {
        wxPostScriptDC dc_ps(NULL, TRUE);
        
        if (dc_ps.Ok())
        {
            Bool gtext = draw_settings->UseGText();
            draw_settings->SetGText(FALSE);
            
            if (fit)
            {
                draw_settings->SetRealWidth(MaxX());
                draw_settings->SetRealHeight(MaxY());
            }

            dc_ps.StartDoc("");
            dc_ps.StartPage();
            Update(dc_ps);
            dc_ps.EndPage();
            dc_ps.EndDoc();
            draw_settings->SetGText(gtext);
        }
    }
}


//****************************************************************************
//*                               SPREAD SHEET                               *
//****************************************************************************

SpreadSheet::SpreadSheet(int _rows, int _cols, int _level, 
                         char *title, wxFrame *parent)
{
    Init(_rows, _cols, _level, title, parent);
}


void SpreadSheet::Init(int rows_, int cols_, int level_, 
                       char *title, wxFrame *parent)
{
    SetDimensions(rows_, cols_);
    level = level_;
    int h, w;
    parent->GetClientSize(&w, &h);
    sheet = new SpreadSheetC(this, parent, 0, 0, w, h-MIN_BUTTON_SPACE);
    
    if (title) label = title;
    else label = " : #"+ToText(level);
}


void SpreadSheet::Clear(void)
{
    for (int i = 1; i <= rows; i++)
        for (int j = 1; j <= cols; j++)
            data(i, j).Clear();
}


void SpreadSheet::SetDimensions(int rows_, int cols_)
{
  assert(rows_ > 0 && cols_ > 0 && "SpreadSheet::Invalid Dimensions");
  rows = rows_;
  cols = cols_;
  data = gRectBlock<SpreadDataCell> (rows, cols);
  row_labels = gBlock<gText>(rows);
  col_labels = gBlock<gText>(cols);
  row_selectable = gBlock<bool>(rows);
  for (int i = 1; i <= rows; row_selectable[i++] = true);
  col_selectable = gBlock<bool>(cols);
  for (int i = 1; i <= cols; col_selectable[i++] = true);
}


void SpreadSheet::AddRow(int row)
{
  if (row == 0) 
    row = rows + 1;

  // add a new row to the matrix
  data.InsertRow(row, (const gArray<SpreadDataCell>)gArray<SpreadDataCell>(cols));

  // Copy the cell types from the previous row
  for (int i = 1; i <= cols; i++) 
    data(rows+1, i).SetType(data(rows, i).GetType());

  row_labels.Insert("", row);
  row_selectable.Insert(true, row);
  rows++;
}


void SpreadSheet::AddCol(int col)
{
  if (col == 0) 
    col = cols + 1;

  // add a new column to the matrix
  data.InsertColumn(col, (const gArray<SpreadDataCell>)gArray<SpreadDataCell>(rows));

  col_labels.Insert("", col);
  col_selectable.Insert(true, col);
  cols++;
}


void SpreadSheet::DelRow(int row)
{
  if (rows < 2) 
    return;
    
  if (row == 0) 
    row = rows;

  // remove a row from the matrix
  data.RemoveRow(row);

  row_labels.Remove(row);
  row_selectable.Remove(row);
  rows--;
}


void SpreadSheet::DelCol(int col)
{
  if (cols < 2) 
    return;
    
  if (col == 0) 
    col = cols;

  // remove a column from the matrix
  data.RemoveColumn(col);

  col_labels.Remove(col);
  col_selectable.Remove(col);
  cols--;
}


Bool SpreadSheet::XYtoRowCol(int x, int y, int *row, int *col) 
{ 
  int orig_row = *row, orig_col = *col;
  if (!sheet->XYtoRowCol(x, y, row, col))
    return FALSE;

  if (row_selectable[*row] && col_selectable[*col])
    return TRUE;
  else {
    *row = orig_row;
    *col = orig_col;
    return FALSE;
  }
}

void SpreadSheet::SetSize(int xs, int ys, int xe, int ye)
{
    sheet->SetSize(xs, ys, xe, ye);
}

void SpreadSheet::Output(gOutput &o) const
{
    for (int i = 1; i <= rows; i++)
    {
        for (int j = 1; j <= cols; j++) 
            o << gPlainText(data(i, j).GetValue()) << ' ';

        o << '\n';
    }
}


//****************************************************************************
//*                           SPREAD SHEET 3D                                *
//****************************************************************************
SpreadSheet3D::SpreadSheet3D(int rows, int cols, int _levels, char *title,
                             wxFrame *parent, unsigned int _features, 
                             SpreadSheetDrawSettings *drs,
                             SpreadSheetDataSettings *dts)
    : wxFrame(parent, title)
{
    assert(rows > 0 && cols > 0 && _levels > 0 && "SpreadSheet3D::Bad Dimensions");

    // Initialize some global GDI objects
    grid_line_pen   = wxThePenList->FindOrCreatePen("BLACK", 1, wxDOT);
    grid_border_pen = wxThePenList->FindOrCreatePen("BLUE", 3, wxSOLID);
    s_selected_pen  = wxThePenList->FindOrCreatePen("GREEN", 2, wxSOLID);
    s_white_pen     = wxThePenList->FindOrCreatePen("WHITE", 2, wxSOLID);
    s_hilight_pen   = wxThePenList->FindOrCreatePen("LIGHT GREY", 2, wxSOLID);
    s_white_brush   = wxTheBrushList->FindOrCreateBrush("WHITE", wxSOLID);
    s_hilight_brush = wxTheBrushList->FindOrCreateBrush("LIGHT GREY", wxSOLID);

    // Initialize the draw settings
    draw_settings = (drs) ? drs : new SpreadSheetDrawSettings(this, cols);
    draw_settings->SetParent(this);
    data_settings = (dts) ? dts : new SpreadSheetDataSettings;

    // Initialize local variables
    toolbar   = 0;
    completed = wxRUNNING;
    editable  = TRUE;
    levels    = _levels;
    label     = title;
    features  = _features;

    // Create the levels,  must do in two steps since gList(int) is not defined
    int i;

    for (i = 1; i <= levels; i++) 
        data.Append((const SpreadSheet)SpreadSheet());

    for (i = 1; i <= levels; i++) 
        data[i].Init(rows, cols, i, 0, this);

    // Turn on level #1
    cur_level = 0;
    SetLevel(1);
    
    if (levels > 1) 
        features |= ANY_BUTTON; // we need a panel for the slider

    MakeFeatures();
    CreateStatusLine(2);

    // Size this frame according to the sheet dimensions
    Resize();
}


SpreadSheet3D::~SpreadSheet3D(void)
{
    Show(FALSE);
    
    if (toolbar)
    {
        delete toolbar;
        toolbar = 0;
    }
    
    if (panel)
    {
        delete panel;
        panel = 0;
    }
    
    if (--draw_settings->ref_cnt == 0) 
        delete draw_settings;
    
    if (--data_settings->ref_cnt == 0) 
        delete data_settings;
}


void SpreadSheet3D::MakeFeatures(void)
{
    panel = 0;
    MakeButtons(features&ALL_BUTTONS);
    SetMenuBar(MakeMenuBar(features&ALL_MENUS));
}


void SpreadSheet3D::MakeButtons(long buttons)
{
    //------------------make the panel---------------------------
    if (buttons) // Create the panel
    {
        if (!panel)
        {
            int h, w;
            panel_x = panel_y = 0;
            panel_new_line = FALSE;
            GetClientSize(&w, &h);
            panel = new wxPanel(this, 0, h-MIN_BUTTON_SPACE, w, 
                                MIN_BUTTON_SPACE, wxBORDER);
        }
        
        if (levels > 1 && !level_item) // create a slider to choose the active level
        {
            level_item = new wxSlider(panel, 
                                      (wxFunction)SpreadSheet3D::spread_slider_func, 
                                      NULL, 1, 1, levels, 140);
            level_item->SetClientData((char *)this);
            panel->NewLine();
            SavePanelPos();
        }
        
        if (buttons & OK_BUTTON) 
            AddButton("OK", (wxFunction)SpreadSheet3D::spread_ok_func);
        
        if (buttons & CANCEL_BUTTON) 
            AddButton("Cancel", (wxFunction)SpreadSheet3D::spread_cancel_func);
        
        if (buttons & PRINT_BUTTON) 
            AddButton("P", (wxFunction)SpreadSheet3D::spread_print_func);
        
        if (buttons & OPTIONS_BUTTON) 
            AddButton("Config", (wxFunction)SpreadSheet3D::spread_options_func);
        
        if (buttons & HELP_BUTTON) 
            AddButton("?", (wxFunction)SpreadSheet3D::spread_help_func);
        
        if (buttons & CHANGE_BUTTON)
        {
            AddButtonNewLine();
            AddButton("Grow/Shrink", (wxFunction)SpreadSheet3D::spread_change_func);
        }
    }
}


void SpreadSheet3D::SetDimensions(int rows_, int cols_, int levels_)
{
    assert(rows_ > 0 && cols_ > 0 && "SpreadSheet3D::Invalid Dimensions");
    int i;
    
    if (GetRows() != rows_ || GetCols() != cols_)
    {
        for (i = 1; i <= levels; i++) 
            data[i].SetDimensions(rows_, cols_);

        DrawSettings()->SetDimensions(rows_, cols_);
    }
    
    if (levels_)
    {
        if (levels_ > levels)
        {
            for (i = 1; i <= levels_-levels; i++) 
                AddLevel();
        }
        
        if (levels_ < levels)
        {
            for (i = 1; i <= levels-levels_; i++) 
                DelLevel();
        }
    }
}


void SpreadSheet3D::OnMenuCommand(int id)
{
    switch (id)
    {
    case OUTPUT_MENU: 
        OnPrint();
        break;

    case CLOSE_MENU: 
        OnOk();
        break;

    case OPTIONS_MENU: 
        DrawSettings()->SetOptions();
        break;

    case CHANGE_MENU: 
        break;

    case HELP_MENU_ABOUT: 
        OnHelp(HELP_MENU_ABOUT);
        break;

    case HELP_MENU_CONTENTS: 
        OnHelp();
        break;

    default: 
        wxMessageBox("Unknown");
        break;
    }
}


void SpreadSheet3D::OnSize(int , int )
{
    int w, h;
    int toolbar_height = (toolbar) ? 40 : 0;
    GetClientSize(&w, &h);
    
    if (toolbar) 
        toolbar->SetSize(0, 0, w, toolbar_height);
    
    if (panel) 
        panel->SetSize(0, h-DrawSettings()->PanelSize(), 
                       w, DrawSettings()->PanelSize());

    //for (int i = 1; i <= data.Length(); i++) data[i].CheckSize();
    for (int i = 1; i <= levels; i++)
    {
        data[i].SetSize(0, toolbar_height, w, 
                        h-DrawSettings()->PanelSize()-toolbar_height);
        data[i].CheckSize();
    }
}


// Callback functions
void SpreadSheet3D::spread_ok_func(wxButton  &ob, wxEvent &)
{
    ((SpreadSheet3D *)ob.GetClientData())->OnOk1();
}


void SpreadSheet3D::OnOk1(void)
{
    OnOk();
}


void SpreadSheet3D::OnOk(void)
{
    SetCompleted(wxOK);
    Show(FALSE);
}


void    SpreadSheet3D::OnCancel1(void)
{
    OnCancel();
}


void    SpreadSheet3D::OnCancel(void)
{
    SetCompleted(wxCANCEL);
    Show(FALSE);
}


void SpreadSheet3D::OnPrint1(void)
{
    OnPrint();
}


void SpreadSheet3D::OnPrint(void)
{
        wxStringList extras("ASCII", NULL);
        wxOutputDialogBox od(&extras);

        if (od.Completed() == wxOK)
        {

            if (!od.ExtraMedia())
            {
                Print(od.GetMedia(), od.GetOption());
            }
            else    // only one extra exists--must be ascii.
            {
                char *s = wxFileSelector("Save", NULL, NULL, NULL, "*.asc", wxSAVE);

                if (s) {
		  gFileOutput out(s);
		  data[cur_level].Output(out);
                }
            }
        }
}



void    SpreadSheet3D::spread_print_func(wxButton   &ob, wxEvent &)
{
    ((SpreadSheet3D *)ob.GetClientData())->OnPrint1();
}


void    SpreadSheet3D::spread_cancel_func(wxButton  &ob, wxEvent &)
{
    ((SpreadSheet3D *)ob.GetClientData())->OnCancel1();
}


void SpreadSheet3D::spread_slider_func(wxSlider &ob, wxCommandEvent &)
{
    ((SpreadSheet3D *)ob.GetClientData())->SetLevel(ob.GetValue());
}


void    SpreadSheet3D::spread_help_func(wxButton    &ob, wxEvent &)
{
    ((SpreadSheet3D *)ob.GetClientData())->OnHelp1();
}


void SpreadSheet3D::OnHelp1(void)
{
    OnHelp();
}


void SpreadSheet3D::OnHelp(int )
{ }


void    SpreadSheet3D::spread_change_func(wxButton  &ob, wxEvent &)
{
    SpreadSheet3D *parent = (SpreadSheet3D *)ob.GetClientData();

    // Create the Grow/Shrink dialog box
    MyDialogBox *gs = new MyDialogBox(0, "Grow/Shrink");

    char *choices[6] =
    {
        "Add Row", "Add Column", "Add Level", "Del Row", "Del Column", "Del Level"
    };
    
    wxRadioBox *rb = new wxRadioBox(gs, NULL, NULL, -1, -1, -1, -1, 6, choices, 2);
    
    if (!parent->DataSettings()->Change(S_CAN_GROW_ROW)) 
        rb->Enable(0, FALSE);
    
    if (!parent->DataSettings()->Change(S_CAN_GROW_COL)) 
        rb->Enable(1, FALSE);
    
    if (!parent->DataSettings()->Change(S_CAN_GROW_LEVEL)) 
        rb->Enable(2, FALSE);
    
    if (!parent->DataSettings()->Change(S_CAN_SHRINK_ROW)) 
        rb->Enable(3, FALSE);
    
    if (!parent->DataSettings()->Change(S_CAN_SHRINK_COL)) 
        rb->Enable(4, FALSE);
    
    if (!parent->DataSettings()->Change(S_CAN_SHRINK_LEVEL)) 
        rb->Enable(5, FALSE);
    
    gs->Go();
    
    if (gs->Completed() == wxOK)
    {
        switch (rb->GetSelection())
        {
        case 0: 
            parent->AddRow();
            break;

        case 1: 
            parent->AddCol();
            break;

        case 2: 
            parent->AddLevel();
            break;

        case 3: 
            parent->DelRow();
            break;

        case 4: 
            parent->DelCol();
            break;

        case 5: 
            parent->DelLevel();
            break;
        }
    }

    delete gs;
}


void SpreadSheet3D::spread_options_func(wxButton &ob, wxEvent &)
{
    SpreadSheet3D *parent = (SpreadSheet3D *)ob.GetClientData();
    parent->DrawSettings()->SetOptions();
}


void SpreadSheet3D::DelLevel(void)
{
    if (levels < 2) 
        return;

    data.Remove(levels);
    levels--;
    delete level_item;
    level_item = new wxSlider(panel, 
                              (wxFunction)SpreadSheet3D::spread_slider_func, 
                              NULL, 1, 1, levels, 140);
    level_item->SetClientData((char *)this);
    Redraw();
}


void SpreadSheet3D::AddLevel(int level)
{
    if (level == 0) 
        level = levels+1;

    data.Insert(SpreadSheet(), level);
    levels++;
    data[level].Init(data[1].GetRows(), data[1].GetCols(), levels, NULL, this);
    level_item = new wxSlider(panel, 
                              (wxFunction)SpreadSheet3D::spread_slider_func, 
                              NULL, 1, 1, levels, 140);
    level_item->SetClientData((char *)this);
    Redraw();
}


void SpreadSheet3D::Output(void)
{
  gFileOutput out("spread.out");
  out << levels << "\n";

  for (int i = 1; i <= levels; i++) {
    data[i].Output(out);
    out << "\n\n";
  }
}


void SpreadSheet3D::SetLevel(int _l)
{
    assert(_l > 0 && _l <= levels);
    
    if (cur_level) 
        data[cur_level].SetActive(FALSE);

    cur_level = _l;
    data[cur_level].SetActive(TRUE);
    SetTitle(label + ":" + data[cur_level].GetLabel());
}


void SpreadSheet3D::SetLabelRow(int row, const gText &s, int level)
{
    if (level == 0)
    {
        for (level = 1; level <= levels; level++) 
            data[level].SetLabelRow(row, s);
    }
    else
        data[level].SetLabelRow(row, s);
}


void SpreadSheet3D::SetLabelCol(int col, const gText &s, int level)
{
    if (level == 0)
    {
        for (level = 1; level <= levels; level++)
            data[level].SetLabelCol(col, s);
    }
    else
        data[level].SetLabelCol(col, s);
}


void SpreadSheet3D::FitLabels(void)
{
    float w, h;
    int max_w = -1, max_h = -1;
    int i;
    
    if (draw_settings->RowLabels())
    {
        for (i = 1; i <= data[1].GetRows(); i++)
        {
            data[1].GetLabelExtent(data[cur_level].GetLabelRow(i), &w, &h);
            
            if (w > max_w) 
                max_w = (int)w;
        }

        draw_settings->SetXStart(max_w+3);
    }
    
    if (draw_settings->ColLabels())
    {
        for (i = 1; i <= data[1].GetCols(); i++)
        {
            data[1].GetLabelExtent(data[cur_level].GetLabelCol(i), &w, &h);
            
            if (h > max_h) 
                max_h = (int)h;
        }

        draw_settings->SetYStart(max_h+3);
    }
}


void SpreadSheet3D::Resize(void)
{
    int w, h, w1 = 0, h1 = 0;
    int toolbar_height = (toolbar) ? 40 : 0;
    
    data[cur_level].GetSize(&w, &h);
    
    if (panel)
    {
        Panel()->Fit();
        Panel()->GetSize(&w1, &h1);
        w = gmax(w, w1);
        h1 = gmax(h1, MIN_BUTTON_SPACE);
        Panel()->SetSize(0, h+toolbar_height, w, h1);
    }

    DrawSettings()->SetPanelSize(h1);
    SetClientSize(w, h+h1+toolbar_height);
}


void SpreadSheet3D::Redraw(void)
{
    char tmp[100];
    int i;
    
    if (data_settings->AutoLabel(S_AUTO_LABEL_ROW))
    {
        for (i = 1; i <= data[1].GetRows(); i++)
        {
            sprintf(tmp, data_settings->AutoLabelStr(S_AUTO_LABEL_ROW), i);
            SetLabelRow(i, tmp);
        }
    }
    
    if (data_settings->AutoLabel(S_AUTO_LABEL_COL))
    {
        for (i = 1; i <= data[1].GetCols(); i++)
        {
            sprintf(tmp, data_settings->AutoLabelStr(S_AUTO_LABEL_COL), i);
            SetLabelCol(i, tmp);
        }
    }

    FitLabels();
    Resize();
}


wxButton *SpreadSheet3D::AddButton(const char *label, wxFunction fun)
{
#ifdef wx_msw
    assert(panel);
    panel->RealAdvanceCursor();
    SavePanelPos();
    
    if (panel_new_line)
    {
        panel_y += 40;
        panel_x = PANEL_LEFT_MARGIN;
        panel_new_line = FALSE;
    }

    wxButton *button = new wxButton(panel, fun, (char *)label, panel_x, panel_y);
#else
    
    if (panel_new_line)
    {
        panel->NewLine();
        panel_new_line = FALSE;
    }

    wxButton *button = new wxButton(panel, fun, (char *)label);
#endif
    button->SetClientData((char *)this);
    return button;
}


wxPanel *SpreadSheet3D::AddPanel(void)
{
#ifdef wx_msw
    panel->RealAdvanceCursor();
    SavePanelPos();
    
    if (panel_new_line)
    {
        panel_y += 40;
        panel_x = PANEL_LEFT_MARGIN;
        panel_new_line = FALSE;
    }

    wxPanel *sub_panel = new wxPanel(panel, panel_x, panel_y);
    return sub_panel;
#else
    //if (panel_new_line) panel->NewLine();
    //wxPanel *sub_panel = new wxPanel(panel);
    panel->NewLine();
    return panel;
#endif
}


/*
  void SpreadSheet3D::AddMenu(wxMenu *submenu, const char *label)
  {
  assert(menubar);      // make sure a menubar exists
  menubar->Append(submenu, (char *)label);
  SetMenuBar(menubar);
  }
*/


wxMenuBar *SpreadSheet3D::MakeMenuBar(long menus)
{
    wxMenuBar *tmp_menubar = 0;

    //-------------------------------make menus----------------------------
    if (menus)
    {
        tmp_menubar = new wxMenuBar;
        wxMenu *file_menu = 0;
        
        if (menus & (OUTPUT_MENU | CLOSE_MENU)) 
            file_menu = new wxMenu;
        
        if (menus & OUTPUT_MENU)
            file_menu->Append(OUTPUT_MENU, "Out&put", "Output to any device");
        
        if (menus & CLOSE_MENU)
            file_menu->Append(CLOSE_MENU, "&Close", "Exit");
        
        if (file_menu) 
            tmp_menubar->Append(file_menu, "&File");

        wxMenu *display_menu = 0;
        
        if (menus & (OPTIONS_MENU | CHANGE_MENU)) 
            display_menu = new wxMenu;

        if (menus & OPTIONS_MENU)
            display_menu->Append(OPTIONS_MENU, "&Options", "Configure display options");

        if (menus & CHANGE_MENU)
            display_menu->Append(CHANGE_MENU, "&Change", "Change sheet dimensions");

        if (display_menu) 
            tmp_menubar->Append(display_menu, "&Display");

        if (menus & HELP_MENU)
        {
            wxMenu *help_menu = new wxMenu;
            help_menu->Append(HELP_MENU_ABOUT, "&About");
            help_menu->Append(HELP_MENU_CONTENTS, "&Contents");
            tmp_menubar->Append(help_menu, "&Help");
        }
    }

    return tmp_menubar;
}


void SpreadSheet3D::SetMenuBar(wxMenuBar *bar)
{
    menubar = bar;

    if (menubar) 
        wxFrame::SetMenuBar(menubar);
}


void SpreadSheet3D::OnSelectedMoved(int , int , SpreadMoveDir )
{ }


// Gui playback code:

void SpreadSheet3D::AddRow(int p_row /*= 0*/)
{
  for (int i = 1; i <= levels; i++)
    data[i].AddRow(p_row);
}

void SpreadSheet3D::AddCol(int p_col /*= 0*/)
{
  for (int i = 1; i <= levels; i++)
    data[i].AddCol(p_col);
  DrawSettings()->AddCol(p_col);
}

void SpreadSheet3D::DelRow(int p_row /*= 0*/)
{
  for (int i = 1; i <= levels; i++)
    data[i].DelRow(p_row);
}

void SpreadSheet3D::DelCol(int p_col /*= 0*/)
{
  for (int i = 1; i <= levels; i++)
    data[i].DelCol(p_col);
  DrawSettings()->DelCol(p_col);
}

void SpreadSheet3D::SetSelectableRow(int p_row, Bool p_selectable)
{
  for (int i = 1; i <= levels; i++)
    data[i].SetSelectableRow(p_row, p_selectable);
}

void SpreadSheet3D::SetSelectableCol(int p_col, Bool p_selectable)
{
  for (int i = 1; i <= levels; i++)
    data[i].SetSelectableCol(p_col, p_selectable);
}
