//+----------------------------------------------------------------------------+
//| Description:  Magic Set Editor - Program to make Magic (tm) cards          |
//| Copyright:    (C) 2001 - 2006 Twan van Laarhoven                           |
//| License:      GNU General Public License 2 or later (see file COPYING)     |
//+----------------------------------------------------------------------------+

// ----------------------------------------------------------------------------- : Includes

#include <gui/control/graph.hpp>
#include <util/alignment.hpp>
#include <gfx/gfx.hpp>
#include <wx/dcbuffer.h>

DECLARE_TYPEOF_COLLECTION(GraphAxisP);
DECLARE_TYPEOF_COLLECTION(GraphElementP);
DECLARE_TYPEOF_COLLECTION(GraphGroup);
typedef map<String,UInt> map_String_UInt;
DECLARE_TYPEOF(map_String_UInt);

// ----------------------------------------------------------------------------- : GraphData

GraphElement::GraphElement(const String& v1) {
	values.push_back(v1);
}
GraphElement::GraphElement(const String& v1, const String& v2) {
	values.push_back(v1);
	values.push_back(v2);
}


GraphData::GraphData(const GraphDataPre& d)
	: axes(d.axes)
{
	// total size
	size = (UInt)d.elements.size();
	// find groups on each axis
	size_t value_count = 1;
	size_t i = 0;
	FOR_EACH(a, axes) {
		map<String,UInt> counts; // note: default constructor for UInt() does initialize to 0
		FOR_EACH_CONST(e, d.elements) {
			counts[e->values[i]] += 1;
		}
		// TODO: allow some ordering in the groups, and allow colors to be passed
		FOR_EACH(c, counts) {
			a->groups.push_back(GraphGroup(c.first, c.second));
			a->max = max(a->max, c.second);
		}
		// find some nice colors for the groups
		if (a->auto_color) {
			double hue = 0.6; // start hue
			bool first = true;
			FOR_EACH(g, a->groups) {
				double amount = double(g.size) / size; // amount this group takes
				if (!first) hue += amount/2;
				g.color = hsl2rgb(hue, 1.0, 0.5);
				hue += amount / 2;
				first = false;
			}
		}
		value_count *= a->groups.size();
		++i;
	}
	// count elements in each position
	values.clear();
	values.resize(value_count, 0);
	FOR_EACH_CONST(e, d.elements) {
		// find index j in elements
		size_t i = 0, j = 0;
		FOR_EACH(a, axes) {
			String v = e->values[i];
			size_t k = 0, l = 0;
			FOR_EACH(g, a->groups) {
				if (v == g.name) {
					k = l;
					break;
				}
				l += 1;
			}
			j = j * a->groups.size() + k;
			++i;
		}
		values[j] += 1;
	}
}


// ----------------------------------------------------------------------------- : Graph1D

void Graph1D::draw(RotatedDC& dc, const vector<int>& current) const {
	draw(dc, axis < current.size() ? current.at(axis) : -1);
}
bool Graph1D::findItem(const RealPoint& pos, const RealRect& rect, vector<int>& out) const {
	int i = findItem(pos, rect);
	if (i == -1) return false;
	else {
		out.clear();
		out.insert(out.begin(), data->axes.size(), -1);
		out.at(axis) = i;
		return true;
	}
}

// ----------------------------------------------------------------------------- : Bar Graph

void BarGraph::draw(RotatedDC& dc, int current) const {
	if (!data) return;
	// Rectangle for bars
	RealRect rect = dc.getInternalRect().move(15, 5, -20, -20);
	// Bar sizes
	GraphAxis& axis = axis_data();
	int count = int(axis.groups.size());
	double width_space = rect.width / count; // including spacing
	double width       = width_space / 5 * 4;
	double space       = width_space / 5;
	double step_height = rect.height / axis.max; // multiplier for bar height
	// Highlight current column
	Color bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
	Color fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
	if (current >= 0) {
		double x = rect.x + width_space * current;
		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(lerp(bg, axis.groups[current].color, 0.25));
		dc.DrawRectangle(RealRect(x + space / 2 - 5, rect.bottom() + 1 + 15,
			                      width + 10, -step_height * axis.groups[current].size - 1.999 - 20));
		dc.SetBrush(lerp(bg, axis.groups[current].color, 0.5));
		dc.DrawRectangle(RealRect(x + space / 2 - 2, rect.bottom() + 1 + 2,
			                      width + 4,  -step_height * axis.groups[current].size - 1.999 - 4));
	}
	// How many labels and lines to draw?
	dc.SetFont(*wxNORMAL_FONT);
	UInt label_step = max(1.0, dc.GetCharHeight() / step_height);
	// Draw backlines (horizontal) and value labels
	dc.SetPen(lerp(bg, fg, 0.5));
	for (UInt i = 0 ; i <= axis.max ; ++i) {
		if (i % label_step == 0) {
			int y = rect.bottom() - i * step_height;
			dc.DrawLine(RealPoint(rect.left() - 2, y), RealPoint(rect.right(), y));
			// draw label, aligned middle right
			String label; label << i;
			RealSize text_size = dc.GetTextExtent(label);
			dc.DrawText(label, align_in_rect(ALIGN_MIDDLE_RIGHT, text_size, RealRect(rect.x - 4, y, 0, 0)));
		}
	}
	// Draw axes
	dc.SetPen(fg);
	dc.DrawLine(rect.bottomLeft() - RealSize(2,0), rect.bottomRight());
	dc.DrawLine(rect.topLeft(),                    rect.bottomLeft());
	// Draw bars
	double x = rect.x;
	FOR_EACH_CONST(g, axis.groups) {
		// draw bar
		dc.SetBrush(g.color);
		dc.DrawRectangle(RealRect(x + space / 2, rect.bottom() + 1, width, -step_height * g.size - 1.999));
		// draw label, aligned bottom center
		RealSize text_size = dc.GetTextExtent(g.name);
		dc.SetClippingRegion(RealRect(x + 2, rect.bottom(), width_space - 4, text_size.height));
		dc.DrawText(g.name, align_in_rect(ALIGN_TOP_CENTER, text_size, RealRect(x, rect.bottom() + 3, width_space, 0)));
		dc.DestroyClippingRegion();
		x += width_space;
	}
}
int BarGraph::findItem(const RealPoint& pos, const RealRect& rect1) const {
	if (!data) return -1;
	// Rectangle for bars
	RealRect rect = rect1.move(15, 5, -20, -20);
	// Bar sizes
	GraphAxis& axis = axis_data();
	int count = int(axis.groups.size());
	double width_space = rect.width / count; // including spacing
	double space       = width_space / 5;
	// Find column in which this point could be located
	int    col    = (pos.x - rect.x) / width_space;
	double in_col = (pos.x - rect.x) - col * width_space;
	if (in_col < space / 2               || // left
	    in_col > width_space - space / 2 || // right
	    pos.y > rect.bottom()            || // below
	    pos.y < rect.top()) {               // above
		return -1;
	}
	if (col < 0 || (size_t)col >= axis_data().groups.size()) {
		return -1;
	} else {
		return col;
	}
}

// ----------------------------------------------------------------------------- : Pie Graph

// ----------------------------------------------------------------------------- : Graph Legend

// ----------------------------------------------------------------------------- : GraphControl

GraphControl::GraphControl(Window* parent, int id)
	: wxControl(parent, id)
{
	graph = new_shared1<BarGraph>(0);
}

void GraphControl::setData(const GraphDataPre& data) {
	setData(new_shared1<GraphData>(data));
}
void GraphControl::setData(const GraphDataP& data) {
	if (graph) {
		graph->setData(data);
		current_item.clear(); // TODO : preserver selection
		Refresh(false);
	}
}

void GraphControl::onPaint(wxPaintEvent&) {
	wxBufferedPaintDC dc(this);
	wxSize cs = GetClientSize();
	RotatedDC rdc(dc, 0, RealRect(RealPoint(0,0),cs), 1, false);
	rdc.SetPen(*wxTRANSPARENT_PEN);
	rdc.SetBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
	rdc.DrawRectangle(rdc.getInternalRect());
	if (graph) graph->draw(rdc, current_item);
}

void GraphControl::onSize(wxSizeEvent&) {
	Refresh(false);
}

void GraphControl::onMouseDown(wxMouseEvent& ev) {
	if (!graph) return;
	wxSize cs = GetClientSize();
	if (graph->findItem(RealPoint(ev.GetX(), ev.GetY()), RealRect(RealPoint(0,0),cs), current_item)) {
		Refresh(false);
	}
}

BEGIN_EVENT_TABLE(GraphControl, wxControl)
	EVT_PAINT		(GraphControl::onPaint)
	EVT_SIZE		(GraphControl::onSize)
	EVT_LEFT_DOWN	(GraphControl::onMouseDown)
END_EVENT_TABLE  ()