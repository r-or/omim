#include "../../base/SRC_FIRST.hpp"
#include "../../3party/sgitess/interface.h"

#include "../../yg/screen.hpp"
#include "../../yg/utils.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/skin.hpp"
#include "../../yg/pen_info.hpp"

#include "../../qt_tstfrm/macros.hpp"

#include "../../testing/testing.hpp"
#include <QtGui/QKeyEvent>

#include "../../base/math.hpp"
#include "../../std/shared_ptr.hpp"

namespace
{
  struct TestDrawPoint
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPoint(m2::PointD(40, 40), 0, yg::EPosCenter, 0);
      p->drawPoint(m2::PointD(40.5, 60), 0, yg::EPosCenter, 0);
      p->drawPoint(m2::PointD(41, 80), 0, yg::EPosCenter, 0);
    }
  };

  struct TestDrawSolidRect
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->immDrawSolidRect(m2::RectF(0, 0, 100, 100), yg::Color(255, 0, 0, 255));
    }
  };

  struct TestDrawLine
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::PointD pts[3] =
      {
        m2::PointD(200, 200),
        m2::PointD(205, 200),
        m2::PointD(290, 200)
      };

//      double pat [] = {7, 7, 10, 10};
      double pat1 [] = {1, 1};
      p->drawPath(pts, 3, p->skin()->mapPenInfo(yg::PenInfo(yg::Color(0xFF, 0xFF, 0xFF, 0xFF), 2, pat1, 2, 0)), 0);
    }
  };

  struct TestDrawPathBase
  {
    std::vector<std::vector<m2::PointD> > m_pathes;
    //std::vector<std::vector<double> > m_patterns;
    std::vector<yg::PenInfo> m_penInfos;
    std::vector<double> m_depthes;

    std::vector<double> m_axisPattern;
    yg::PenInfo m_axisPenInfo;
    bool m_drawAxis;

    void Init()
    {
      m_drawAxis = false;

      m_axisPattern.push_back(2);
      m_axisPattern.push_back(2);
      m_axisPenInfo = yg::PenInfo(yg::Color(0xFF, 0, 0, 0xFF), 2, &m_axisPattern[0], m_axisPattern.size(), 0);
    }

    void AddTest(std::vector<m2::PointD> const & points,
        std::vector<double> const & pattern,
        yg::Color const & color = yg::Color(255, 255, 255, 255),
        double width = 2,
        double depth = 0,
        double offset = 0)
    {
      m_pathes.push_back(points);
      //m_patterns.push_back(pattern);
      m_penInfos.push_back(yg::PenInfo(color, width, pattern.empty() ? 0 : &pattern[0], pattern.size(), offset));
      m_depthes.push_back(depth);
    }

    std::vector<m2::PointD> & GetTestPoints(size_t i)
    {
      return m_pathes[i];
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      for (size_t i = 0; i < m_pathes.size(); ++i)
      {
        p->drawPath(&m_pathes[i][0], m_pathes[i].size(), p->skin()->mapPenInfo(m_penInfos[i]), m_depthes[i]);
        if (m_drawAxis)
          p->drawPath(&m_pathes[i][0], m_pathes[i].size(), p->skin()->mapPenInfo(m_axisPenInfo), m_depthes[i]);
      }
    }

    void makeStar(vector<m2::PointD> & pts, m2::RectD const & r)
    {
      pts.push_back(m2::PointD(r.minX(), r.maxY()));
      pts.push_back(m2::PointD(r.Center().x, r.minY()));
      pts.push_back(m2::PointD(r.maxX(), r.maxY()));
      pts.push_back(m2::PointD(r.minX(), r.minY() + r.SizeY() / 3));
      pts.push_back(m2::PointD(r.maxX(), r.minY() + r.SizeY() / 3));
      pts.push_back(m2::PointD(r.minX(), r.maxY()));
    }

  };

  struct TestDrawPathWithSkinPageMiss : public TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();
      m_drawAxis = false;

      vector<m2::PointD> points;
      vector<double> pattern;

      size_t const columns = 30;
      size_t const rows = 6;

      for (size_t j = 0; j < rows; ++j)
      {
        for (size_t i = 0; i < columns; ++i)
        {
          points.clear();
          points.push_back(m2::PointD(100 * j + 10, i * 15 + 20));
          points.push_back(m2::PointD(100 * j + 100, i * 15 + 20));
          AddTest(points, pattern, yg::Color(128 + 128 / columns * i, 128 + 128 / rows * j, 0, 255), rand() % 15);
        }
      }
    }
  };

  struct PathWithOffset : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      m_drawAxis = true;

      vector<m2::PointD> pts;
      vector<double> pattern;

      pts.push_back(m2::PointD(100, 100));
      pts.push_back(m2::PointD(200, 100));

      pattern.push_back(20);
      pattern.push_back(30);

      /// The path should start from -10px offset.
//      AddTest(pts, pattern, yg::Color(0, 0, 255, 255), 4, 0, -10);

      pts.clear();
      pts.push_back(m2::PointD(100, 200));
      pts.push_back(m2::PointD(200, 200));

      /// The path should start from 60px offset.
      AddTest(pts, pattern, yg::Color(0, 0, 255, 255), 4, 0, 60);
    }
  };

  struct TestDrawPathJoin : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;
    bool OnKeyPress(QKeyEvent * kev)
    {
      if (kev->key() == Qt::Key_Up)
      {
        GetTestPoints(0).back().y--;
        return true;
      }
      if (kev->key() == Qt::Key_Down)
      {
        GetTestPoints(0).back().y++;
        return true;
      }
      if (kev->key() == Qt::Key_Left)
      {
        GetTestPoints(0).back().x--;
        return true;
      }
      if (kev->key() == Qt::Key_Right)
      {
        GetTestPoints(0).back().x++;
        return true;
      }

      return false;
    }

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.push_back(m2::PointD(220, 300));
      testPoints.push_back(m2::PointD(280, 240));
      testPoints.push_back(m2::PointD(320, 369));
      testPoints.push_back(m2::PointD(420, 370));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40);

      testPoints.clear();
      testPoints.push_back(m2::PointD(320, 300));
      testPoints.push_back(m2::PointD(380, 240));
      testPoints.push_back(m2::PointD(420, 369));
      testPoints.push_back(m2::PointD(520, 370));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40);

//      testPoints.clear();
//
//      testPoints.push_back(m2::PointD(460, 100));
//      testPoints.push_back(m2::PointD(505, 200));
//      testPoints.push_back(m2::PointD(600, 150));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 255, 255), 40);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 120));
//      testPoints.push_back(m2::PointD(160, 120));
//
//      testPattern.clear();
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(50, 200));
//      testPoints.push_back(m2::PointD(50, 350));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 120));
//      testPoints.push_back(m2::PointD(250, 120));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(400, 200));
//      testPoints.push_back(m2::PointD(400, 350));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
//
//      testPoints.clear();
//      testPoints.push_back(m2::PointD(100, 200));
//      testPoints.push_back(m2::PointD(240, 220));
//
//      AddTest(testPoints, testPattern, yg::Color(0, 0, 0, 255), 60);
    }
  };

  struct TestDrawPathSolidWithZ : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(200, 200));
      testPoints.push_back(m2::PointD(300, 200));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40, 0.5);

//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);
//      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(150, 220));
      testPoints.push_back(m2::PointD(300, 220));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40, 0.5);

      testPattern.clear();
      testPoints.clear();

      testPoints.push_back(m2::PointD(200, 240));
      testPoints.push_back(m2::PointD(300, 240));

      AddTest(testPoints, testPattern, yg::Color(0, 0, 255, 255), 40, 0);
    }
  };

  struct TestDrawPathSolidWithClipRect : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(200, 200));
      testPoints.push_back(m2::PointD(0, 0));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40, 0.5);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->enableClipRect(true);
      p->setClipRect(m2::RectI(50, 70, 150, 150));
      base_t::DoDraw(p);
      p->endFrame();
      m2::RectI r(0, 0, 0, 0);
      p->beginFrame();
      p->setClipRect(r);
      p->enableClipRect(true);
      p->clear();
    }
  };

  struct TestDrawPathSolidDiffWidth : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      vector<m2::PointD> testPoints;

      int starCount = 10;
      m2::PointD starSize(50, 50);
      m2::PointD pt(10, 10);

      for (int i = 0; i < starCount; ++i)
      {
        base_t::makeStar(testPoints, m2::RectD(pt, pt + starSize));
        AddTest(testPoints, vector<double>(), yg::Color(255, 0, 0, 255), i + 1);
        pt = pt + m2::PointD(starSize.x + i + 3, 0);
        testPoints.clear();
      }
    }
  };

  struct TestDrawPathSolid1PX : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(120, 200));
      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 1);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 1);
    }
  };

  struct TestDrawPathSolid2PX : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

      testPoints.push_back(m2::PointD(120, 200));
      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 2);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 2);
    }
  };


  struct TestDrawPathSolid : TestDrawPathBase
  {
    typedef TestDrawPathBase base_t;

    void Init()
    {
      base_t::Init();

      std::vector<m2::PointD> testPoints;
      std::vector<double> testPattern;

//      testPoints.push_back(m2::PointD(120, 200));
//      testPoints.push_back(m2::PointD(180, 140));
      testPoints.push_back(m2::PointD(220, 269));
      testPoints.push_back(m2::PointD(320, 270));

      AddTest(testPoints, testPattern, yg::Color(255, 0, 0, 255), 40);

      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);
      testPattern.push_back(20);

      testPoints.clear();
      testPoints.push_back(m2::PointD(420, 300));
      testPoints.push_back(m2::PointD(480, 240));
      testPoints.push_back(m2::PointD(520, 369));
      testPoints.push_back(m2::PointD(620, 370));

      AddTest(testPoints, testPattern, yg::Color(0, 255, 0, 255), 40);
    }
  };

  struct TestDrawPoly
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
//      m2::PointD ptsStrip[5] = {m2::PointD(10, 10), m2::PointD(40, 10), m2::PointD(70, 10), m2::PointD(90, 60), m2::PointD(130, 30)};
//      p->drawTriangles(ptsStrip, 5, yg::TriangleStrip, p->skin()->mapColor(yg::Color(255, 0, 0, 255)));

//      m2::PointD ptsFan[5] = {m2::PointD(150, 20), m2::PointD(170, 80), m2::PointD(190, 100), m2::PointD(200, 80), m2::PointD(220, 60)};
//      p->drawTriangles(ptsFan, 5, yg::TriangleFan, p->skin()->mapColor(yg::Color(0, 255, 0, 255)));

      m2::PointD ptsList[6] = {m2::PointD(20, 80), m2::PointD(50, 120), m2::PointD(80, 80), m2::PointD(110, 80), m2::PointD(140, 120), m2::PointD(80, 120)};
      p->drawTrianglesList(ptsList, 6, /*yg::TriangleList, */p->skin()->mapColor(yg::Color(0, 0, 255, 255)), 0);
    }
  };

  /// Trying to draw polygon with more vertices that fits into internal buffer.
  struct TestDrawPolyOverflow
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      size_t verticesCount = 30000;
      vector<m2::PointD> vertices;

      double r = 200;
      m2::PointD offsetPt(300, 300);

      m2::PointD centerPt(0, 0);
      m2::PointD prevPt(r, 0);

      double const Angle = 2 * 3.1415 / verticesCount;
      double const sinA = sin(Angle);
      double const cosA = cos(Angle);

      for (size_t i = 0; i < verticesCount; ++i)
      {
        vertices.push_back(centerPt + offsetPt);

        m2::PointD nextPt(prevPt.x * cosA + prevPt.y * sinA, -prevPt.x * sinA + prevPt.y * cosA);

        vertices.push_back(prevPt + offsetPt);
        vertices.push_back(nextPt + offsetPt);

        prevPt = nextPt;
      }

      p->drawTrianglesList(&vertices[0],
                           vertices.size(),
                           p->skin()->mapColor(yg::Color(0, 0, 255, 255)), 0);
    }
  };

  struct TestDrawText
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::PointD path[2] = {m2::PointD(100, 200), m2::PointD(1000, 200)};
      double pat[2] = {2, 2};
      p->drawPath(path, sizeof(path) / sizeof(m2::PointD), p->skin()->mapPenInfo(yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, pat, 2, 0)), 0);

      p->drawText(m2::PointD(200, 200), 0                , 20, yg::Color(0, 0, 0, 0), "0", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(240, 200), math::pi / 2     , 20, yg::Color(0, 0, 0, 0), "0", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(280, 200), math::pi         , 20, yg::Color(0, 0, 0, 0), "0", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(320, 200), math::pi * 3 / 2 , 20, yg::Color(0, 0, 0, 0), "0", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(360, 200), math::pi / 18, 20, yg::Color(0, 0, 0, 0), "0", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(40, 50), math::pi / 18, 20, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication", true, yg::Color(255, 255, 255, 255), 0, false, true);
    }
  };

  struct TestDrawSingleSymbol
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0, 20, yg::Color(0, 0, 0, 0), "X", true, yg::Color(255, 255, 255, 255), 1, false, true);
    }
  };

  struct TestDrawEmptySymbol
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0, 20, yg::Color(0, 0, 0, 0), " ", true, yg::Color(255, 255, 255, 255), 1, false, true);
    }
  };

  struct TestDrawStringOnString
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      size_t const maxTimes = 10;
      size_t const yStep = 30;

      for (size_t i = 0; i < maxTimes; ++i)
        for (size_t j = 1; j <= i+1; ++j)
          p->drawText(m2::PointD(40, 10 + yStep * i), math::pi / 6, 20, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication", true, yg::Color(255, 255, 255, 255), 0, false, true);
    }
  };

  struct TestDrawSingleSymbolAndSolidPath
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      vector<m2::PointD> path;
      path.push_back(m2::PointD(40, 50));
      path.push_back(m2::PointD(70, 50));

      double pat[] = { 2, 2 };
      yg::PenInfo penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
      yg::PenInfo solidPenInfo = yg::PenInfo(yg::Color(0xFF, 0, 0, 0xFF), 4, 0, 0, 0);

      p->drawText(m2::PointD(40, 50), 0, 20, yg::Color(0, 0, 0, 0), "S", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawPath(&path[0], path.size(), p->skin()->mapPenInfo(solidPenInfo), 0);

    }
  };

  struct TestDrawString
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0/*, math::pi / 18*/, 20, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication", true, yg::Color(255, 255, 255, 255), 0, false, true);
    }
  };

  struct TestDrawStringWithFixedFont
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0/*, math::pi / 18*/, 12, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication", true, yg::Color(255, 255, 255, 255), 0, true, true);
    }
  };

  struct TestDrawStringWithColor
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0/*, math::pi / 18*/, 25, yg::Color(0, 0, 255, 255), "Simplicity is the ultimate sophistication", true, yg::Color(255, 255, 255, 255), 0, false, true);
    }
  };


  struct TestDrawUnicodeSymbols
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawText(m2::PointD(40, 50), 0, 12, yg::Color(0, 0, 0, 0), "Latin Symbol : A", true, yg::Color(255, 255, 255, 255), 0, false, true);
      p->drawText(m2::PointD(40, 80), 0, 12, yg::Color(0, 0, 0, 0), "Cyrillic Symbol : Ы", true, yg::Color(255, 255, 255, 255), 0, false, true);
    }
  };

  struct TestDrawTextRectWithFixedFont : TestDrawStringWithFixedFont
  {
    typedef TestDrawStringWithFixedFont base_t;
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      m2::RectD r = p->textRect("Simplicity is the ultimate sophistication", 12, true, false);

      m2::PointD startPt(40, 50);

      m2::PointD pts[6] = {
        startPt + m2::PointD(r.minX(), r.minY()),
        startPt + m2::PointD(r.maxX(), r.minY()),
        startPt + m2::PointD(r.maxX(), r.maxY()),
        startPt + m2::PointD(r.minX(), r.minY()),
        startPt + m2::PointD(r.maxX(), r.maxY()),
        startPt + m2::PointD(r.minX(), r.maxY())
      };

      p->drawTrianglesList(pts, 6, p->skin()->mapColor(yg::Color(0, 0, 255, 255)), 0);

      base_t::DoDraw(p);
    }
  };


  double calc_length(vector<m2::PointD> const & v)
  {
    double ret = 0.0;
    for (size_t i = 1; i < v.size(); ++i)
      ret += v[i-1].Length(v[i]);
    return ret;
  }

  struct TestDrawTextOnPath
  {
    std::vector<m2::PointD> m_path;
    std::string m_text;
    yg::PenInfo m_penInfo;

    TestDrawTextOnPath()
    {
      m_path.push_back(m2::PointD(40, 200));
      m_path.push_back(m2::PointD(100, 100));
      m_path.push_back(m2::PointD(300, 100));
      m_path.push_back(m2::PointD(400, 300));
      m_text = "Simplicity is the ultimate sophistication. Leonardo Da Vinci.";

      double pat[] = { 2, 2 };
      m_penInfo = yg::PenInfo(yg::Color(0, 0, 0, 0xFF), 2, &pat[0], ARRAY_SIZE(pat), 0);
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), p->skin()->mapPenInfo(m_penInfo), 0);
      p->drawPathText(&m_path[0], m_path.size(), 10, yg::Color(0, 0, 0, 0), m_text, calc_length(m_path), 0.0, yg::gl::Screen::middle_line, true, yg::Color(255, 255, 255, 255), 0, false);
    }
  };

  struct TestDrawTextOnPathWithOffset : TestDrawTextOnPath
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawPath(&m_path[0], m_path.size(), p->skin()->mapPenInfo(m_penInfo), 0);

      double const len = calc_length(m_path);
      p->drawPathText(&m_path[0], m_path.size(), 10, yg::Color(0, 0, 0, 0), m_text, len, 0.0, yg::gl::Screen::above_line, true, yg::Color(255, 255, 255, 255), 0, false);
      p->drawPathText(&m_path[0], m_path.size(), 10, yg::Color(0, 0, 0, 0), m_text, len, 0.0, yg::gl::Screen::under_line, true, yg::Color(255, 255, 255, 255), 0, false);
    }
  };

  struct TestDrawTextOverflow
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        p->drawText(m2::PointD(10, startY), 0, startSize + i, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication. Leonardo Da Vinci", true, yg::Color(255, 255, 255, 255), 0, false, true);
        startY += startSize + i;
      }
    }
  };

  struct TestDrawTextFiltering
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      int const startSize = 20;
      size_t const sizesCount = 20;

      int startY = 30;
      for (size_t i = 0; i < sizesCount; ++i)
      {
        p->drawText(m2::PointD(10, startY), 0, startSize/* + i*/, yg::Color(0, 0, 0, 0), "Simplicity is the ultimate sophistication. Leonardo Da Vinci", true, yg::Color(255, 255, 255, 255), 100, false, true);
        p->drawText(m2::PointD(5, startY + (startSize + i) / 2), 0, startSize/* + i*/, yg::Color(0, 0, 0, 0), "This text should be filtered", true, yg::Color(255, 255, 255, 255), 100, false, true);
        startY += startSize + i;
      }
    }
  };

  struct TestDrawRandomTextFiltering
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      char const * texts [] = {"Simplicity is the ultimate sophistication", "Leonardo Da Vinci"};

      int startSize = 20;
      int endSize = 40;

      int textsCount = 200;

      for (int i = 0; i < textsCount; ++i)
        p->drawText(
            m2::PointD(rand() % 500, rand() % 500),
            0,
            rand() % (endSize - startSize) + startSize,
            yg::Color(rand() % 255, rand() % 255, rand() % 255, 255),
            texts[rand() % (sizeof(texts) / sizeof(char*))],
            true,
            yg::Color(255, 255, 255, 255),
            rand() % 10,
            false,
            true);
    }
  };

  struct TestDrawUtilsRect
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      shared_ptr<yg::gl::RGBA8Texture> texture(new yg::gl::RGBA8Texture(512, 512));
      texture->randomize();

      p->immDrawRect(
          m2::RectF(0, 0, 512, 512),
          m2::RectF(0, 0, 1, 1),
          texture,
          true,
          yg::Color(255, 0, 0, 255),
          false);
    }
  };

  struct TestDrawUtilsRectFilledTexture
  {
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      shared_ptr<yg::gl::RGBA8Texture> texture(new yg::gl::RGBA8Texture(512, 512));
      texture->fill(yg::Color(0, 255, 0, 255));

      p->immDrawRect(
          m2::RectF(0, 0, 512, 512),
          m2::RectF(0, 0, 1, 1),
          texture,
          true,
          yg::Color(255, 0, 0, 255),
          false);
    }
  };

  struct TestDrawSector
  {
  public:
    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      p->drawArc(m2::PointD(100, 100), 0, math::pi * 2, 30, yg::Color(0, 0, 255, 128), 12000);
      p->fillSector(m2::PointD(100, 100), 0, math::pi * 2, 30, yg::Color(0, 0, 255, 64), 12000);
    }
  };

  struct TestDrawSGIConvex
  {
    tess::VectorDispatcher m_d;
    std::vector<tess::Vertex> m_vertices;
    TestDrawSGIConvex()
    {
      m_vertices.push_back(tess::Vertex(100, 100));
      m_vertices.push_back(tess::Vertex(300, 100));
      m_vertices.push_back(tess::Vertex(250, 120));
      m_vertices.push_back(tess::Vertex(300, 300));
      m_vertices.push_back(tess::Vertex(100, 300));
      m_vertices.push_back(tess::Vertex(180, 250));
      m_vertices.push_back(tess::Vertex(120, 200));
      m_vertices.push_back(tess::Vertex(60, 150));
      m_vertices.push_back(tess::Vertex(100, 100));

      tess::Tesselator t;
      t.setDispatcher(&m_d);
      t.setBoundaryOnly(false);
      t.setWindingRule(tess::WindingNonZero);
      t.beginPolygon();
      t.beginContour();
      for (size_t i = 0; i < m_vertices.size(); ++i)
        t.add(m_vertices[i]);
      t.endContour();
      t.endPolygon();
    }

    void DoDraw(shared_ptr<yg::gl::Screen> p)
    {
      double inputDataPat[] = {10, 0};
      yg::PenInfo inputDataRule(yg::Color::fromARGB(0xFF000000), 6, inputDataPat, 2, 0);

      double triangleFanPat[] = {10, 10};
      yg::PenInfo triangleFanRule(yg::Color::fromARGB(0xFFFF0000), 5, triangleFanPat, 2, 0);

      double triangleStripPat[] = {10, 10};
      yg::PenInfo triangleStripRule(yg::Color::fromARGB(0xFF00FF00), 4, triangleStripPat, 2, 0);

      double triangleListPat[] = {10, 10};
      yg::PenInfo triangleListRule(yg::Color::fromARGB(0xFF0000FF), 3, triangleListPat, 2, 0);

      double lineLoopPat[] = {2, 2};
      yg::PenInfo lineLoopRule(yg::Color::fromARGB(0xFF00FFFF), 2, lineLoopPat, 2, 0);

      uint32_t inputDataID = p->skin()->mapPenInfo(inputDataRule);
      uint32_t triangleFanID = p->skin()->mapPenInfo(triangleFanRule);
      /*uint32_t triangleStripID = */p->skin()->mapPenInfo(triangleStripRule);
      uint32_t triangleListID = p->skin()->mapPenInfo(triangleListRule);
      uint32_t lineLoopID = p->skin()->mapPenInfo(lineLoopRule);

      p->drawPath((m2::PointD const *)&m_vertices[0], m_vertices.size(), inputDataID, 0);

      for (size_t i = 0; i < m_d.indices().size(); ++i)
      {
        std::vector<std::vector<m2::PointD > > poly;

        switch (m_d.indices()[i].first)
        {
        case tess::TrianglesFan:
          {
            for (size_t j = 2; j < m_d.indices()[i].second.size(); ++j)
            {
              poly.push_back(std::vector<m2::PointD>());
              int first = m_d.indices()[i].second[0];
              int second = m_d.indices()[i].second[1];
              int third = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), triangleFanID, 0);
            break;
          }
        case tess::TrianglesList:
          {
            for (size_t j = 0; j < m_d.indices()[i].second.size() / 3; ++j)
            {
              poly.push_back(std::vector<m2::PointD>());

              int first = m_d.indices()[i].second[j * 3];
              int second = m_d.indices()[i].second[j * 3 + 1];
              int third = m_d.indices()[i].second[j * 3 + 2];

              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), triangleListID, 0);
            break;
          }
        case tess::TrianglesStrip:
          {
            for (size_t j = 2; j < m_d.indices()[i].second.size(); ++j)
            {
              poly.push_back(std::vector<m2::PointD>());
              int first = m_d.indices()[i].second[j - 2];
              int second = m_d.indices()[i].second[j - 1];
              int third = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[second].x, m_d.vertices()[second].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[third].x, m_d.vertices()[third].y));
              poly.back().push_back(m2::PointD(m_d.vertices()[first].x, m_d.vertices()[first].y));
            }

            for (size_t j = 0; j < poly.size(); ++j)
              p->drawPath(&poly[j][0], poly[j].size(), triangleFanID, 0);
            break;
          }
        case tess::LineLoop:
          {
            poly.push_back(std::vector<m2::PointD>());
            for (size_t j = 0; j < m_d.indices()[i].second.size(); ++j)
            {
              int idx = m_d.indices()[i].second[j];
              poly.back().push_back(m2::PointD(m_d.vertices()[idx].x, m_d.vertices()[idx].y));
            }

            poly.back().push_back(poly.back()[0]);

            p->drawPath(&poly[0][0], poly[0].size(), lineLoopID, 0);
            break;
          }
        }
      }
    }
  };

  struct TestDrawSymbolFiltering
  {
    void DoDraw(shared_ptr<yg::gl::Screen> const & p)
    {
      for (int i = 0; i < 40; ++i)
        p->drawSymbol(m2::PointD(100 + i, 100), p->skin()->mapSymbol("hospital"), yg::EPosCenter, 0);
    }
  };

//   UNIT_TEST_GL(TestDrawPolyOverflow);
//   UNIT_TEST_GL(TestDrawText);
//   UNIT_TEST_GL(TestDrawSingleSymbol);
//   UNIT_TEST_GL(TestDrawEmptySymbol);
//   UNIT_TEST_GL(TestDrawSingleSymbolAndSolidPath);
//   UNIT_TEST_GL(TestDrawString);
//   UNIT_TEST_GL(TestDrawStringWithFixedFont);
//   UNIT_TEST_GL(TestDrawStringWithColor);
//   UNIT_TEST_GL(TestDrawUnicodeSymbols);
//   UNIT_TEST_GL(TestDrawTextRectWithFixedFont);
//   UNIT_TEST_GL(TestDrawStringOnString);
//   UNIT_TEST_GL(TestDrawTextOnPath);
//   UNIT_TEST_GL(TestDrawTextOnPathWithOffset);
//   UNIT_TEST_GL(TestDrawTextOverflow);
//   UNIT_TEST_GL(TestDrawTextFiltering);
//   UNIT_TEST_GL(TestDrawRandomTextFiltering);
//   UNIT_TEST_GL(TestDrawSGIConvex);
//   UNIT_TEST_GL(TestDrawPoly);
//   UNIT_TEST_GL(TestDrawSolidRect);
//   UNIT_TEST_GL(TestDrawPathWithSkinPageMiss);
//   UNIT_TEST_GL(TestDrawPathWithOffset);
//   UNIT_TEST_GL(TestDrawPathJoin);
//   UNIT_TEST_GL(TestDrawPathSolid1PX);
//   UNIT_TEST_GL(TestDrawPathSolid2PX);
//   UNIT_TEST_GL(TestDrawPathSolid);
//   UNIT_TEST_GL(TestDrawSector);
//   UNIT_TEST_GL(TestDrawPathSolidDiffWidth);
//   UNIT_TEST_GL(TestDrawPathSolidWithZ);
//   UNIT_TEST_GL(TestDrawPathSolidWithClipRect);
//   UNIT_TEST_GL(TestDrawUtilsRect);
//   UNIT_TEST_GL(TestDrawUtilsRectFilledTexture);
   UNIT_TEST_GL(TestDrawSymbolFiltering);
}
