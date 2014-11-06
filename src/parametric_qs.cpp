#include "parametric_qs.h"
#include "parametric.h"
#include "main.h"
#include "casewin.h"
#include "numericvareditform.h"
#include <wex/utils.h>


enum {
  ID_GroupBox1,
  ID_Label3,
  ID_grpOutline,
  ID_lstValues,
  ID_lstVariables,
  ID_Label1,
  ID_btnEditValues,
  ID_Label2,
  ID_Cancel,
  ID_OK,
  ID_btnRemoveVar,
  ID_btnAddVar };

BEGIN_EVENT_TABLE( Parametric_QS, wxPanel )
	EVT_BUTTON( ID_btnAddVar, Parametric_QS::OnAddVariable )
	EVT_BUTTON( ID_btnRemoveVar, Parametric_QS::OnRemoveVariable )
	EVT_LISTBOX( ID_lstVariables, Parametric_QS::OnVariableSelect )
	EVT_LISTBOX_DCLICK( ID_lstVariables, Parametric_QS::OnVarDblClick)
	EVT_BUTTON( ID_btnEditValues, Parametric_QS::OnEditValues )
	EVT_LISTBOX_DCLICK( ID_lstValues, Parametric_QS::OnValueDblClick)
END_EVENT_TABLE()

Parametric_QS::Parametric_QS(wxWindow *parent, Case *c)
: wxPanel(parent), m_case(c)
{
	btnAddVar = new wxButton(this, ID_btnAddVar, "Add");
	btnRemoveVar = new wxButton(this, ID_btnRemoveVar, "Remove");
	btnEditValues = new wxButton(this, ID_btnEditValues, "Edit");
	wxArrayString _data_lstVariables;

	lstVariables = new wxListBox(this, ID_lstVariables, wxPoint(-1, -1), wxSize(-1, -1), _data_lstVariables, wxLB_SINGLE);
	wxArrayString _data_lstValues;
	lstValues = new wxListBox(this, ID_lstValues, wxPoint(-1, -1), wxSize(-1, -1), _data_lstValues, wxLB_SINGLE);

	Label1 = new wxStaticText(this, ID_Label1, "Variables:");
	Label2 = new wxStaticText(this, ID_Label2, "Selected Variable Values:");


	wxFlexGridSizer *fgs = new wxFlexGridSizer(2, 2, 5, 25);

	wxBoxSizer *bsvars = new wxBoxSizer(wxHORIZONTAL);
	bsvars->Add(Label1, 0, wxALIGN_CENTER, 0);
	bsvars->AddStretchSpacer();
	bsvars->Add(btnAddVar, 0);
	bsvars->Add(btnRemoveVar, 0);

	wxBoxSizer *bsvals = new wxBoxSizer(wxHORIZONTAL);
	bsvals->Add(Label2, 0, wxALIGN_CENTER, 0);
	bsvals->AddStretchSpacer();
	bsvals->Add(btnEditValues, 0);

	fgs->Add(bsvars, 3, wxEXPAND | wxALL, 0);
	fgs->Add(bsvals, 3, wxEXPAND | wxALL, 0);
	fgs->Add(lstVariables, 3, wxEXPAND | wxALL, 0);
	fgs->Add(lstValues, 3, wxEXPAND | wxALL, 0);

	fgs->AddGrowableCol(0, 1);
	fgs->AddGrowableCol(1, 1);
	fgs->AddGrowableRow(1, 1);

	SetSizer(fgs);

}


void Parametric_QS::UpdateFromParametricData()
{
	RefreshVariableList();
	RefreshValuesList();
}


void Parametric_QS::OnEditValues(wxCommandEvent &evt)
{
	if ( !m_case)
		return;

	int idx = lstVariables->GetSelection();
	if (idx < 0 || idx > m_input_names.Count())
		wxMessageBox("No variable selected!");
	else
	{
		wxString name = m_input_names[idx];
		wxArrayString values = GetValuesList(name);
		VarInfo *varinfo = m_case->Variables().Lookup(name);
		if (varinfo)
		{
			if (ShowEditValuesDialog(
					"Edit Parametric Values for '" + varinfo->Label +
					((varinfo->Units !="") ? (" ("+ varinfo->Units +")'") :"'"),
					values, name) )
			{
				SetValuesList(name, values);
				RefreshValuesList();
			}
		}
		
	}
}

bool Parametric_QS::ShowFixedDomainDialog(const wxString &title,
	const wxArrayString &names, const wxArrayString &labels, wxArrayString &list,
	bool expand_all)
{
	SelectVariableDialog dlg(this, title);
	dlg.SetItems(names, labels);
	dlg.SetCheckedNames(list);
	if (expand_all)
		dlg.ShowAllItems();

	if (dlg.ShowModal() == wxID_OK)
	{
		wxArrayString names = dlg.GetCheckedNames();

		// remove any from list
		int i = 0;
		while (i<(int)list.Count())
		{
			if (names.Index(list[i]) < 0)
				list.RemoveAt(i);
			else
				i++;
		}

		// append any new ones
		for (i = 0; i<(int)names.Count(); i++)
		{
			if (list.Index(names[i]) < 0)
				list.Add(names[i]);
		}


		return true;
	}
	else
		return false;
}


bool Parametric_QS::ShowEditValuesDialog(const wxString &title,
	wxArrayString &values, const wxString &varname)
{

	VarInfo *vi = m_case->Variables().Lookup(varname);
	if (!vi)
		return false;
	VarValue *vv = m_case->Values().Get(varname);
	if (!vv)
		return false;

	int i;
	int vvtype = vv->Type();
	int vitype = vi->Type;


	if (vvtype == VV_NUMBER
		&& vi->IndexLabels.Count() > 0)
	{
		// fixed domain selection (combo box, list, radio choice etc)
		wxArrayString fixed_items = vi->IndexLabels;
		wxArrayString cur_items;
		for (i = 0; i<(int)values.Count(); i++)
		{
			int item_i = atoi(values[i].c_str());
			if (item_i >= 0 && item_i < (int)fixed_items.Count())
				cur_items.Add(fixed_items[item_i]);
		}

		if (ShowFixedDomainDialog(title, fixed_items, fixed_items, cur_items, true))
		{
			// translate back to integer values
			values.Clear();
			for (int i = 0; i<(int)cur_items.Count(); i++)
				values.Add(wxString::Format("%d", fixed_items.Index(cur_items[i])));

			return true;
		}
		else
			return false;
	}
	else if (vitype == VF_LIBRARY)
	{
		// get lib item list (climate or lib list)
		wxArrayString fixed_items;
		/*
		if (v->GetExpression() == "climates")
			fixed_items = LibGetClimateList(mCase->GetSamFile());
		else if (v->GetExpression() == "wind_files")
		{
			wxArrayString ext;
			ext.Add("srw");
			fixed_items = LibGetClimateList(mCase->GetSamFile(), ext);
		}
		else if (v->GetExpression().Left(7) == "liblist")
			fixed_items = LibGetEntriesForLibraryType(v->GetExpression().Mid(8));

		return ShowFixedDomainDialog(title, fixed_items, values);
		*/
	}
	else if (vvtype == VV_NUMBER)
	{
		return ShowNumericValuesDialog(title, values);
	}
	/*
	else if (vtype == VAR_STRING && v->GetDataSource() == ::VDSRC_INPUT && v->GetExpression() != "")
	{
		// STRING combo box
		wxArrayString fixed_items = Split(v->GetExpression(), ",");
		return ShowFixedDomainDialog(title, fixed_items, values);
	}
	*/

	wxMessageBox("Could not edit values for \"" + vi->Label + "\" (domain type error)");
	return false;
}


bool Parametric_QS::ShowNumericValuesDialog(const wxString &title,
	wxArrayString &values)
{
	NumericVarEditFormDialog dlg(this, title);
	NumericVarEditForm *frm = dlg.GetPanel();
	frm->SetValues(values, false);

	if (dlg.ShowModal() == wxID_OK)
	{
		values = frm->lstValues->GetStrings();
		return true;
	}
	else
		return false;
}


void Parametric_QS::OnValueDblClick(wxCommandEvent &evt)
{
	OnEditValues(evt);
}


void Parametric_QS::OnRemoveVariable(wxCommandEvent &evt)
{
	if ( !m_case)
		return;

	int idx = lstVariables->GetSelection();
	if (idx < 0)
		wxMessageBox("No variable selected!");
	else
	{
		wxString name = "";
		if ((idx > 0) && (idx < m_input_names.Count()))
			name = m_input_names[idx];

		for (std::vector<wxArrayString>::iterator it = m_input_values.begin();
			it != m_input_values.end(); ++it)
		{
			if ((*it).Item(0) == name)
			{
				m_input_values.erase(it);
				break;
			}
		}

		m_input_names.RemoveAt(idx);
	}

	RefreshVariableList();

	if (lstVariables->GetCount() > 0)
		lstVariables->Select(idx-1 >= 0 ? idx-1 : idx );

	RefreshValuesList();

}

void Parametric_QS::OnAddVariable(wxCommandEvent &evt)
{
	if ( !m_case )
		return;

	wxArrayString names, labels;
	wxString case_name(SamApp::Project().GetCaseName(m_case));

	ConfigInfo *ci = m_case->GetConfiguration();
	VarInfoLookup &vil = ci->Variables;

	for (VarInfoLookup::iterator it = vil.begin(); it != vil.end(); ++it)
	{
		wxString name = it->first;
		VarInfo &vi = *(it->second);

		// update to select only "Parametric" variables
		if (vi.Flags & VF_PARAMETRIC)
		{
			wxString label = vi.Label;
			if (label.IsEmpty())
				label = "{ " + name + " }";
			if (!vi.Units.IsEmpty())
				label += " (" + vi.Units + ")";
			if (!vi.Group.IsEmpty())
				label = vi.Group + "/" + label;

			labels.Add(label);
			names.Add(name);
		}
	}

	wxSortByLabels(names, labels);
	SelectVariableDialog dlg(this, "Select Inputs");
	dlg.SetItems(names, labels);
	dlg.SetCheckedNames(m_input_names);
	if (dlg.ShowModal() == wxID_OK)
	{
		m_input_names = dlg.GetCheckedNames();
		RefreshVariableList();
	}
}



void Parametric_QS::OnVariableSelect(wxCommandEvent &evt)
{
	RefreshValuesList();
}

void Parametric_QS::OnVarDblClick(wxCommandEvent &evt)
{
	RefreshValuesList();
	OnEditValues(evt);
}

void Parametric_QS::RefreshValuesList()
{
	if (!m_case)
		return;


	wxArrayString items;

	int idx = lstVariables->GetSelection();
	
	if (idx >= 0 && idx < m_input_names.Count())
	{
		wxString name = m_input_names[idx];
		items = GetValuesList( name );
		if (items.Count() == 0) // add base case value
		{
			wxArrayString values;
			values.Add(name);
			wxString val = GetBaseCaseValue(name);
			values.Add(val);
			items.Add(val);
			m_input_values.push_back(values);
		}
	}
	
	lstValues->Freeze();
	lstValues->Clear();
	lstValues->Append(items);
	lstValues->Thaw();
}


wxString Parametric_QS::GetBaseCaseValue(const wxString &varname)
{
	wxString val;
	VarValue *vv = m_case->Values().Get(varname);
	if (vv)
		val = vv->AsString();
	return val;
}


wxArrayString Parametric_QS::GetValuesList(const wxString &varname)
{
	wxArrayString list;
	for (int i = 0; i < m_input_values.size(); i++)
	{
		if (m_input_values[i].Count() > 0 && m_input_values[i].Item(0) == varname)
		{
			for (int j = 1; j < m_input_values[i].Count(); j++)
				list.Add(m_input_values[i].Item(j));
			break;
		}
	}
	return list;
}

void Parametric_QS::SetValuesList(const wxString &varname, const wxArrayString &values)
{
	int idx = -1;
	if (values.Count() <= 0) return;
	for (int i = 0; i < m_input_values.size(); i++)
	{
		if (m_input_values[i].Count() > 0 && m_input_values[i].Item(0) == varname)
		{
			idx = i;
			break;
		}
	}
	wxArrayString vals;
	vals.Add(varname);
	for (int i = 0; i < values.Count(); i++)
		vals.Add(values[i]);
	if (idx > -1)
		m_input_values[idx] = vals;
	else
		m_input_values.push_back(vals);
}

void Parametric_QS::UpdateCaseParametricData()
{
	ParametricData &par = m_case->Parametric();
	par.ClearRuns();
	par.Setup.clear();
	int num_runs = 1;
	for (int i = 0; i < m_input_values.size(); i++)
	{
		num_runs *= m_input_values[i].Count() - 1;
	}
	for (int i = 0; i < m_input_names.Count(); i++)
	{
		std::vector<VarValue> vvv;
		ParametricData::Var pv;
		for (int num_run = 0; num_run < num_runs; num_run++)
		{ // add values for inputs only
			if (VarValue *vv = m_case->Values().Get(m_input_names[i]))
				vvv.push_back(*vv);
		}
		pv.Name = m_input_names[i];
		pv.Values = vvv;
		par.Setup.push_back(pv);
	}
	for (int num_run =0; num_run < num_runs; num_run++)
	{
		Simulation *s = new Simulation(m_case, wxString::Format("Parametric #%d", (int)(num_run + 1)));
		par.Runs.push_back(s);
	}
	// set values - can do this once and set num_runs
	int repeat = 1;
	for (int col = 0; col < m_input_names.Count(); col++)
	{
		int row = 0;
		wxArrayString vals = GetValuesList(m_input_names[col]);
		while (row < num_runs - 1)
		{
			for (int j = 0; j < vals.Count(); j++)
			{
				for (int k = 0; k < repeat; k++)
				{
					wxString value = vals[j];
					VarValue *vv = &par.Setup[col].Values[row];
					VarValue::Parse(vv->Type(), value, *vv);
					row++;
				}
			}
		}
		repeat *= vals.Count();
	}
}
	


void Parametric_QS::RefreshVariableList()
{
	if (!m_case)
		return;

	lstVariables->Freeze();
	lstVariables->Clear();
	
	for (int i=0;i<m_input_names.Count();i++)
	{
		VarInfo *vi = m_case->Variables().Lookup(m_input_names[i]);
		if (!vi)
		{
			lstVariables->Append("<<Label Lookup Error>>");
			continue;
		}
		wxString suffix = "";

		if (!vi->Units.IsEmpty())
			suffix += " (" + vi->Units + ") ";

		if (!vi->Group.IsEmpty())
			lstVariables->Append(vi->Group + "/" + vi->Label + suffix);
		else
			lstVariables->Append( vi->Label + suffix);
	}
	

	lstVariables->Thaw();

	lstValues->Clear();
}



BEGIN_EVENT_TABLE( Parametric_QSDialog, wxDialog )
	EVT_CLOSE(Parametric_QSDialog::OnClose)
	EVT_BUTTON(ID_OK, Parametric_QSDialog::OnCommand)
	EVT_BUTTON(ID_Cancel, Parametric_QSDialog::OnCommand)
	END_EVENT_TABLE()

	Parametric_QSDialog::Parametric_QSDialog(wxWindow *parent, const wxString &title, Case *c)
	: wxDialog(parent, -1, title, wxDefaultPosition, wxDefaultSize, wxRESIZE_BORDER | wxDEFAULT_DIALOG_STYLE)
{
//	ParametricData par(c);// set par based on case
	mPanel = new Parametric_QS(this, c);
	wxBoxSizer *button_sizer = new wxBoxSizer(wxHORIZONTAL);
	button_sizer->AddStretchSpacer();
	button_sizer->Add(new wxButton(this, ID_OK, "OK"));
	button_sizer->Add(new wxButton(this, ID_Cancel, "Cancel"));
	wxBoxSizer *main_sizer = new wxBoxSizer(wxVERTICAL);
	main_sizer->Add(mPanel, 2, wxALL | wxEXPAND, 10);
	main_sizer->Add(button_sizer, 0, wxALIGN_RIGHT | wxRIGHT | wxBOTTOM, 10);
	SetSizerAndFit(main_sizer);
}


void Parametric_QSDialog::OnCommand(wxCommandEvent &evt)
{
	switch (evt.GetId())
	{
	case ID_Cancel:
		EndModal(wxID_CANCEL);
		Destroy();
		break;
	case ID_OK:
		mPanel->UpdateCaseParametricData();
		EndModal(wxID_OK);
		Destroy();
		break;
	}
}

void Parametric_QSDialog::OnClose(wxCloseEvent &evt)
{
	EndModal(wxID_CANCEL);
	Destroy();
}

