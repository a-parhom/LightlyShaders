#include "liblshelper_export.h"

#include <effect/effecthandler.h>
#include <QRegion>
#include <QImage>
#include <QPainterPath>

template <typename T> int signum(T val) {
    return (T(0) < val) - (val < T(0));
}

namespace KWin {

class LIBLSHELPER_EXPORT LSHelper: public QObject
{
    Q_OBJECT

	public:
		LSHelper();
		~LSHelper();

		void reconfigure();
		QPainterPath superellipse(float size, int n, int translate);
    	QImage genMaskImg(int size, bool mask, bool outer_rect);
		void roundBlurRegion(EffectWindow *w, Region *region);
		void roundBlurRegion(EffectWindow *w, RegionF *region);
		bool isManagedWindow(EffectWindow *w);
		void blurWindowAdded(EffectWindow *w);
		void blurWindowDeleted(EffectWindow *w);
		int roundness();

		enum { RoundedCorners = 0, SquircledCorners };
		enum { TopLeft = 0, TopRight, BottomRight, BottomLeft, NTex };

		Region* m_maskRegions[NTex];

	private:
		bool hasShadow(EffectWindow *w);
		void setMaskRegions();
		Region* createMaskRegion(QImage img, int size, int corner);

		int m_size, m_cornersType, m_squircleRatio, m_shadowOffset;
		bool m_disabledForMaximized;
		QList<EffectWindow *> m_managed;
};

} //namespace
