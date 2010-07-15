#include "CSGTermRenderer.h"
#include "visitor.h"
#include "state.h"
#include "csgterm.h"
#include "module.h"
#include "csgnode.h"
#include "transformnode.h"
#include "rendernode.h"
#include "printutils.h"

#include <string>
#include <map>
#include <list>
#include <sstream>
#include <iostream>
#include <assert.h>

/*!
	\class CSGTermRenderer

	A visitor responsible for creating a tree of CSGTerm nodes used for rendering
	with OpenCSG.
*/

void CSGTermRenderer::applyToChildren(const AbstractNode &node, CSGTermRenderer::CsgOp op)
{
	if (this->visitedchildren[node.index()].size() == 0) return;

	CSGTerm *t1 = NULL;
	for (ChildList::const_iterator iter = this->visitedchildren[node.index()].begin();
			 iter != this->visitedchildren[node.index()].end();
			 iter++) {
		const AbstractNode *chnode = *iter;
		CSGTerm *t2 = this->stored_term[chnode->index()];
		this->stored_term.erase(chnode->index());
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			if (op == UNION) {
				t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
			} else if (op == DIFFERENCE) {
				t1 = new CSGTerm(CSGTerm::TYPE_DIFFERENCE, t1, t2);
			} else if (op == INTERSECTION) {
				t1 = new CSGTerm(CSGTerm::TYPE_INTERSECTION, t1, t2);
			}
		}
	}
	if (t1 && node.modinst->tag_highlight && this->highlights) {
		this->highlights->push_back(t1->link());
	}
	if (t1 && node.modinst->tag_background && this->background) {
		this->background->push_back(t1);
// FIXME: don't store in stored_term? 		return NULL;
	}
	this->stored_term[node.index()] = t1;
}

Response CSGTermRenderer::visit(const State &state, const AbstractNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermRenderer::visit(const State &state, const AbstractIntersectionNode &node)
{
	if (state.isPostfix()) {
		applyToChildren(node, INTERSECTION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

static CSGTerm *render_csg_term_from_ps(double m[20], 
																				QVector<CSGTerm*> *highlights, 
																				QVector<CSGTerm*> *background, 
																				PolySet *ps, 
																				const ModuleInstantiation *modinst, 
																				int idx)
{
	CSGTerm *t = new CSGTerm(ps, m, QString("n%1").arg(idx));
	if (modinst->tag_highlight && highlights)
		highlights->push_back(t->link());
	if (modinst->tag_background && background) {
		background->push_back(t);
		return NULL;
	}
	return t;
}

Response CSGTermRenderer::visit(const State &state, const AbstractPolyNode &node)
{
	if (state.isPostfix()) {
		PolySet *ps = node.render_polyset(AbstractPolyNode::RENDER_OPENCSG);
		CSGTerm *t1 = render_csg_term_from_ps(m, this->highlights, this->background, ps, node.modinst, node.index());
		this->stored_term[node.index()] = t1;
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermRenderer::visit(const State &state, const CsgNode &node)
{
	if (state.isPostfix()) {
		CsgOp op;
		switch (node.type) {
		case CSG_TYPE_UNION:
			op = UNION;
			break;
		case CSG_TYPE_DIFFERENCE:
			op = DIFFERENCE;
			break;
		case CSG_TYPE_INTERSECTION:
			op = INTERSECTION;
			break;
		}
		applyToChildren(node, op);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

Response CSGTermRenderer::visit(const State &state, const TransformNode &node)
{
	if (state.isPostfix()) {
		double x[20];
		
		for (int i = 0; i < 16; i++)
		{
			int c_row = i%4;
			int m_col = i/4;
			x[i] = 0;
			for (int j = 0; j < 4; j++)
				x[i] += c[c_row + j*4] * m[m_col*4 + j];
		}
		
		for (int i = 16; i < 20; i++)
			x[i] = m[i] < 0 ? c[i] : m[i];

		// FIXME: Apply the x matrix.
		// FIXME: Look into how bottom-up vs. top down affects matrix handling

		applyToChildren(node, UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

// FIXME: Find out how to best call into CGAL from this visitor
Response CSGTermRenderer::visit(const State &state, const RenderNode &node)
{
	PRINT("WARNING: Found render() statement but compiled without CGAL support!");
	if (state.isPostfix()) {
		applyToChildren(node, UNION);
		addToParent(state, node);
	}
	return ContinueTraversal;
}

/*!
	Adds ourself to out parent's list of traversed children.
	Call this for _every_ node which affects output during the postfix traversal.
*/
void CSGTermRenderer::addToParent(const State &state, const AbstractNode &node)
{
	assert(state.isPostfix());
	this->visitedchildren.erase(node.index());
	if (state.parent()) {
		this->visitedchildren[state.parent()->index()].push_back(&node);
	}
}


#if 0

// FIXME: #ifdef ENABLE_CGAL
#if 0
CSGTerm *CgaladvNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	if (type == MINKOWSKI)
		return render_csg_term_from_nef(m, highlights, background, "minkowski", this->convexity);

	if (type == GLIDE)
		return render_csg_term_from_nef(m, highlights, background, "glide", this->convexity);

	if (type == SUBDIV)
		return render_csg_term_from_nef(m, highlights, background, "subdiv", this->convexity);

	return NULL;
}

#else // ENABLE_CGAL

CSGTerm *CgaladvNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	PRINT("WARNING: Found minkowski(), glide() or subdiv() statement but compiled without CGAL support!");
	return NULL;
}

#endif // ENABLE_CGAL



// FIXME: #ifdef ENABLE_CGAL
#if 0
CSGTerm *AbstractNode::render_csg_term_from_nef(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background, const char *statement, int convexity) const
{
	QString key = mk_cache_id();
	if (PolySet::ps_cache.contains(key)) {
		PRINT(PolySet::ps_cache[key]->msg);
		return AbstractPolyNode::render_csg_term_from_ps(m, highlights, background,
				PolySet::ps_cache[key]->ps->link(), modinst, idx);
	}

	print_messages_push();
	CGAL_Nef_polyhedron N;

	QString cache_id = mk_cache_id();
	if (cgal_nef_cache.contains(cache_id))
	{
		PRINT(cgal_nef_cache[cache_id]->msg);
		N = cgal_nef_cache[cache_id]->N;
	}
	else
	{
		PRINTF_NOCACHE("Processing uncached %s statement...", statement);
		// PRINTA("Cache ID: %1", cache_id);
		QApplication::processEvents();

		QTime t;
		t.start();

		N = this->renderCSGMesh();

		int s = t.elapsed() / 1000;
		PRINTF_NOCACHE("..rendering time: %d hours, %d minutes, %d seconds", s / (60*60), (s / 60) % 60, s % 60);
	}

	PolySet *ps = NULL;

	if (N.dim == 2)
	{
		DxfData dd(N);
		ps = new PolySet();
		ps->is2d = true;
		dxf_tesselate(ps, &dd, 0, true, false, 0);
		dxf_border_to_ps(ps, &dd);
	}

	if (N.dim == 3)
	{
		if (!N.p3.is_simple()) {
			PRINTF("WARNING: Result of %s() isn't valid 2-manifold! Modify your design..", statement);
			return NULL;
		}

		ps = new PolySet();
		
		CGAL_Polyhedron P;
		N.p3.convert_to_Polyhedron(P);

		typedef CGAL_Polyhedron::Vertex Vertex;
		typedef CGAL_Polyhedron::Vertex_const_iterator VCI;
		typedef CGAL_Polyhedron::Facet_const_iterator FCI;
		typedef CGAL_Polyhedron::Halfedge_around_facet_const_circulator HFCC;

		for (FCI fi = P.facets_begin(); fi != P.facets_end(); ++fi) {
			HFCC hc = fi->facet_begin();
			HFCC hc_end = hc;
			ps->append_poly();
			do {
				Vertex v = *VCI((hc++)->vertex());
				double x = CGAL::to_double(v.point().x());
				double y = CGAL::to_double(v.point().y());
				double z = CGAL::to_double(v.point().z());
				ps->append_vertex(x, y, z);
			} while (hc != hc_end);
		}
	}

	if (ps)
	{
		ps->convexity = convexity;
		PolySet::ps_cache.insert(key, new PolySet::ps_cache_entry(ps->link()));

		CSGTerm *term = new CSGTerm(ps, m, QString("n%1").arg(idx));
		if (modinst->tag_highlight && highlights)
			highlights->push_back(term->link());
		if (modinst->tag_background && background) {
			background->push_back(term);
			return NULL;
		}
		return term;
	}
	print_messages_pop();

	return NULL;
}

CSGTerm *RenderNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	return render_csg_term_from_nef(m, highlights, background, "render", this->convexity);
}

#else

CSGTerm *RenderNode::render_csg_term(double m[20], QVector<CSGTerm*> *highlights, QVector<CSGTerm*> *background) const
{
	CSGTerm *t1 = NULL;
	PRINT("WARNING: Found render() statement but compiled without CGAL support!");
	foreach(AbstractNode * v, children) {
		CSGTerm *t2 = v->render_csg_term(m, highlights, background);
		if (t2 && !t1) {
			t1 = t2;
		} else if (t2 && t1) {
			t1 = new CSGTerm(CSGTerm::TYPE_UNION, t1, t2);
		}
	}
	if (modinst->tag_highlight && highlights)
		highlights->push_back(t1->link());
	if (t1 && modinst->tag_background && background) {
		background->push_back(t1);
		return NULL;
	}
	return t1;
}

#endif



#endif

