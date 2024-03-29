#include "Session.h"
#include "ui_Controls.h"
#include "ShapeItem.h"

Session::Session(   Scene *scene, Controls * control,
                    Matcher * matcher, Blender * blender, QObject *parent) : QObject(parent),
                    s(scene), c(control), m(matcher), b(blender), curTaskIndex(-1)
{
    this->connect( control, SIGNAL(hideAll()), SLOT(hideAll()) );
    this->connect( control, SIGNAL(showSelect()), SLOT(showSelect()));
    this->connect( control, SIGNAL(showMatch()), SLOT(showMatch()));
    this->connect( control, SIGNAL(showCreate()), SLOT(showCreate()));

	/// per-page connections
	// Matcher:
	matcher->connect(control->ui->autoButton, SIGNAL(clicked()), SLOT(autoMode()));
	matcher->connect(control->ui->manualButton, SIGNAL(clicked()), SLOT(manualMode()));
	matcher->connect(control->ui->groupingButton, SIGNAL(clicked()), SLOT(groupingMode()));
	matcher->connect(scene, SIGNAL(mousePressDownEvent(QGraphicsSceneMouseEvent*)), SLOT(mousePress(QGraphicsSceneMouseEvent*)));
	matcher->connect(scene, SIGNAL(mousePressUpEvent(QGraphicsSceneMouseEvent*)), SLOT(mouseRelease(QGraphicsSceneMouseEvent*)));
	control->connect(matcher, SIGNAL(correspondenceFromFile()), SLOT(forceManualMatch()));
	control->connect(matcher, SIGNAL(switchedToManual()), SLOT(forceManualMatch()));

	// Blender:
	blender->connect(control->ui->exportButton, SIGNAL(clicked()), SLOT(exportSelected()));
	blender->connect(control->ui->jobButton, SIGNAL(clicked()), SLOT(saveJob()));
    //blender->connect(control->ui->filterOption, SIGNAL(stateChanged(int)), SLOT(filterStateChanged(int)));
	blender->connect(scene, SIGNAL(mousePressDownEvent(QGraphicsSceneMouseEvent*)), SLOT(mousePress(QGraphicsSceneMouseEvent*)));
	control->connect(blender, SIGNAL(blendStarted()), SLOT(disableTabs()));
	control->connect(blender, SIGNAL(blendDone()), SLOT(enableTabs()));
    control->connect(blender, SIGNAL(blendFinished()), SLOT(enableTabs()));
}

void Session::shapeChanged(int i, QGraphicsItem * shapeItem)
{
    // Clean up if existing
    if(s->inputGraphs[i])
    {
        s->removeItem(s->inputGraphs[i]);
        delete s->inputGraphs[i];
    }
    s->inputGraphs[i] = NULL;

    ShapeItem * item = (ShapeItem *) shapeItem;

    QString graphFile = item->property["graph"].toString();

    s->inputGraphs[i] = new GraphItem(new Structure::Graph(graphFile), s->graphRect(i), s->camera);
	s->inputGraphs[i]->name = item->property["name"].toString();
    s->addItem(s->inputGraphs[i]);
    Structure::Graph * graph = s->inputGraphs[i]->g;

	// Connection
	m->connect(s->inputGraphs[i], SIGNAL(hit(GraphItem::HitResult)), SLOT(graphHit(GraphItem::HitResult)));
	m->connect(s, SIGNAL(rightClick()), SLOT(clearMatch()));
	m->connect(s, SIGNAL(doubleClick()), SLOT(setMatch()));

    // Visualization options
    graph->property["showNodes"] = false;
    foreach(Structure::Node * n, graph->nodes)
    {
        n->vis_property["meshColor"].setValue( QColor(180,180,180) );
        n->vis_property["meshSolid"].setValue( true );
    }

    emit( update() );
}

void Session::showSelect()
{
	// Invalidate the corresponder
	m->gcorr->deleteLater();
	m->gcorr = NULL;
}

void Session::showMatch(){
	m->show();
}

void Session::showCreate(){
	b->show();
}

void Session::hideAll(){
    if(m->isVisible()) m->hide();
    if(b->isVisible()) b->hide();
}

void Session::displayNextTask()
{
	// Special case: last task
	if(curTaskIndex == tasks.size() - 1)
	{
		this->connect(s->doFadeOut(), SIGNAL(finished()), SLOT(emitAllTasksDone()));
		return;
	}

	// Regular case
	this->connect(s->doFadeOut(), SIGNAL(finished()), SLOT(advanceTasks()));

	curTaskIndex += 1;
}

void Session::advanceTasks()
{
	StudyTask curTask = tasks[curTaskIndex];

	for(int i = 0; i < curTask.data.size(); i++)
	{
		PropertyMap itemData = curTask.data[i];

		ShapeItem * item = new ShapeItem;
		item->property["name"] = itemData["graphFile"];
		item->property["graph"] = itemData["graphFile"];

		// Thumbnail
		QPixmap thumbnail( itemData["thumbFile"].toString() );
		item->property["image"].setValue(thumbnail);
		item->width = thumbnail.width();
		item->height = thumbnail.height();
		this->shapeChanged(i, item);
	}

	//s->doFadeIn();
}

void Session::emitAllTasksDone()
{
	emit( allTasksDone() );
}
