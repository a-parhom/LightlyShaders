#include "liblshelper_export.h"

#include <effect/effecthandler.h>
#include <QRegion>
#include <QImage>
#include <QPainterPath>
#include <memory>

template <typename T>
int signum(T val)
{
	return (T(0) < val) - (val < T(0));
}

namespace KWin
{

	class LIBLSHELPER_EXPORT LSHelper : public QObject
	{
		Q_OBJECT

	public:
		LSHelper();
		~LSHelper();

		void reconfigure();
		QPainterPath superellipse(float size, int n, int translate);
		QImage genMaskImg(int size, bool mask, bool outer_rect);
		void roundBlurRegion(EffectWindow *w, QRegion *region);
		bool isManagedWindow(EffectWindow *w);
		void blurWindowAdded(EffectWindow *w);
		void blurWindowDeleted(EffectWindow *w);
		int roundness();

		enum
		{
			RoundedCorners = 0,
			SquircledCorners
		};
		enum
		{
			TopLeft = 0,
			TopRight,
			BottomRight,
			BottomLeft,
			NTex
		};

		// Método seguro para acessar regiões de máscara
		const QRegion *getMaskRegion(int corner) const
		{
			if (corner < 0 || corner >= NTex)
				return nullptr;
			return m_maskRegionsSmart[corner].get();
		}

	private:
		std::unique_ptr<QRegion> m_maskRegionsSmart[NTex];

	private:
		bool hasShadow(EffectWindow *w) const;
		void setMaskRegions();
		QRegion createMaskRegion(QImage img, int size, int corner);

		// Métodos auxiliares para isManagedWindow (reduzem complexidade)
		bool isSpecialWindowType(EffectWindow *w) const;
		bool isBlacklistedClass(EffectWindow *w) const;
		bool isSpecialApplicationCase(EffectWindow *w) const;

		int m_size = 0;
		int m_cornersType = 0;
		int m_squircleRatio = 0;
		int m_shadowOffset = 0;
		bool m_disabledForMaximized = false;
		QList<EffectWindow *> m_managed;
	};

} // namespace