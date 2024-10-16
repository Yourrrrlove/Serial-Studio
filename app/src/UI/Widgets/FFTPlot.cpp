/*
 * Copyright (c) 2020-2023 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "UI/Dashboard.h"
#include "Misc/ThemeManager.h"
#include "UI/Widgets/FFTPlot.h"

/**
 * @brief Constructor to initialize the FFTPlot.
 * @param index Index of the FFT plot data.
 */
Widgets::FFTPlot::FFTPlot(int index)
  : m_size(0)
  , m_index(index)
  , m_transformer(0, QStringLiteral("Hann"))
{
  // Get pointers to Serial Studio modules
  auto dash = &UI::Dashboard::instance();

  // Validate index
  if (m_index < 0 || m_index >= dash->fftCount())
    return;

  // Configure layout
  m_layout.addWidget(&m_plot);
  m_layout.setContentsMargins(8, 8, 8, 8);
  setLayout(&m_layout);

  // Configure x-axis
  auto xAxisEngine = m_plot.axisScaleEngine(QwtPlot::xBottom);
  xAxisEngine->setAttribute(QwtScaleEngine::Floating, true);

  // Attach curve to plot
  m_curve.attach(&m_plot);

  // Initialize FFT size
  auto dataset = dash->getFFT(m_index);
  int size = qMax(8, dataset.fftSamples());
  while (m_transformer.setSize(size) != QFourierTransformer::FixedSize)
    --size;
  m_size = size;

  // Obtain sampling rate from dataset
  m_samplingRate = dataset.fftSamplingRate();
  m_plot.setAxisScale(QwtPlot::xBottom, 0, m_samplingRate / 2);

  // Allocate FFT and sample arrays
  m_fft.reset(new float[m_size]);
  m_samples.reset(new float[m_size]);
  m_curve.setSamples(QVector<QPointF>(m_size, QPointF(0, 0)));

  // Configure plot axes and titles
  m_plot.setFrameStyle(QFrame::Plain);
  m_plot.setAxisScale(QwtPlot::yLeft, -100, 0);
  m_plot.setAxisTitle(QwtPlot::xBottom, tr("Frequency (Hz)"));
  m_plot.setAxisTitle(QwtPlot::yLeft, tr("Magnitude (dB)"));

  // Configure visual style
  onThemeChanged();
  connect(&Misc::ThemeManager::instance(), &Misc::ThemeManager::themeChanged,
          this, &Widgets::FFTPlot::onThemeChanged);

  // Connect update signal
  onAxisOptionsChanged();
  connect(dash, &UI::Dashboard::updated, this, &FFTPlot::updateData,
          Qt::DirectConnection);
  connect(dash, &UI::Dashboard::axisVisibilityChanged, this,
          &FFTPlot::onAxisOptionsChanged, Qt::DirectConnection);

  // Plot data at 20 Hz
  connect(&Misc::TimerEvents::instance(), &Misc::TimerEvents::timeout20Hz, this,
          [=] {
            if (m_replot && isEnabled())
            {
              m_plot.replot();
              m_replot = false;
            }
          });
}

/**
 * @brief Slot to update the FFT data.
 */
void Widgets::FFTPlot::updateData()
{
  // Check if widget is disabled
  if (!isEnabled())
    return;

  // Update FFT data and plot
  auto plotData = UI::Dashboard::instance().fftPlotValues();
  if (plotData.count() > m_index)
  {
    // Obtain samples from data
    auto data = plotData.at(m_index);
    for (int i = 0; i < m_size; ++i)
      m_samples[i] = static_cast<float>(data[i]);

    // Obtain FFT transformation
    m_transformer.forwardTransform(m_samples.data(), m_fft.data());
    m_transformer.rescale(m_fft.data());

    // Create a final array with the magnitudes for each point
    qreal maxMagnitude = 0;
    QVector<QPointF> points(m_size / 2);
    for (int i = 0; i < m_size / 2; ++i)
    {
      const qreal re = m_fft[i];
      const qreal im = m_fft[m_size / 2 + i];
      const qreal magnitude = sqrt(re * re + im * im);
      const qreal frequency
          = static_cast<qreal>(i) * m_samplingRate / static_cast<qreal>(m_size);
      points[i] = QPointF(frequency, magnitude);
      if (magnitude > maxMagnitude)
        maxMagnitude = magnitude;
    }

    // Convert to decibels
    for (int i = 0; i < m_size / 2; ++i)
    {
      const qreal re = m_fft[i];
      const qreal im = m_fft[m_size / 2 + i];
      const qreal magnitude = sqrt(re * re + im * im) / maxMagnitude;
      const qreal dB = (magnitude > 0) ? 20 * log10(magnitude)
                                       : -INFINITY; // Avoid log10(0)
      const qreal frequency
          = static_cast<qreal>(i) * m_samplingRate / static_cast<qreal>(m_size);
      points[i] = QPointF(frequency, dB);
    }

    // Update curve with new data
    m_replot = true;
    m_curve.setSamples(points);
  }
}

/**
 * Updates the widget's visual style and color palette to match the colors
 * defined by the application theme file.
 */
void Widgets::FFTPlot::onThemeChanged()
{
  // Set window palette
  auto theme = &Misc::ThemeManager::instance();
  QPalette palette;
  palette.setColor(QPalette::Base,
                   theme->getColor(QStringLiteral("widget_base")));
  palette.setColor(QPalette::Window,
                   theme->getColor(QStringLiteral("widget_window")));
  setPalette(palette);

  // Set plot palette
  palette.setColor(QPalette::Base,
                   theme->getColor(QStringLiteral("widget_base")));
  palette.setColor(QPalette::Highlight,
                   theme->getColor(QStringLiteral("widget_highlight")));
  palette.setColor(QPalette::Text,
                   theme->getColor(QStringLiteral("widget_text")));
  palette.setColor(QPalette::ButtonText,
                   theme->getColor(QStringLiteral("widget_text")));
  palette.setColor(QPalette::WindowText,
                   theme->getColor(QStringLiteral("widget_text")));
  palette.setColor(QPalette::Dark,
                   theme->getColor(QStringLiteral("groupbox_hard_border")));
  palette.setColor(QPalette::Light,
                   theme->getColor(QStringLiteral("groupbox_hard_border")));
  m_plot.setPalette(palette);
  m_plot.setCanvasBackground(
      theme->getColor(QStringLiteral("groupbox_background")));

  // Get curve color
  const auto colors = theme->colors()["widget_colors"].toArray();
  const auto color = colors.count() > m_index
                         ? colors.at(m_index).toString()
                         : colors.at(colors.count() % m_index).toString();

  // Set curve color & plot style
  m_curve.setPen(QColor(color), 2, Qt::SolidLine);
}

/**
 * @brief Updates the visibility of the plot axes based on user-selected axis
 *        options.
 *
 * This function responds to changes in axis visibility settings from the
 * dashboard. Depending on the user’s selection, it will set the visibility of
 * the X and/or Y axes on the plot.
 *
 * @see UI::Dashboard::axisVisibility()
 * @see QwtPlot::setAxisVisible()
 */
void Widgets::FFTPlot::onAxisOptionsChanged()
{
  switch (UI::Dashboard::instance().axisVisibility())
  {
    case UI::Dashboard::AxisXY:
      m_plot.setAxisVisible(QwtPlot::yLeft, true);
      m_plot.setAxisVisible(QwtPlot::xBottom, true);
      break;
    case UI::Dashboard::AxisXOnly:
      m_plot.setAxisVisible(QwtPlot::yLeft, false);
      m_plot.setAxisVisible(QwtPlot::xBottom, true);
      break;
    case UI::Dashboard::AxisYOnly:
      m_plot.setAxisVisible(QwtPlot::yLeft, true);
      m_plot.setAxisVisible(QwtPlot::xBottom, false);
      break;
    case UI::Dashboard::NoAxesVisible:
      m_plot.setAxisVisible(QwtPlot::yLeft, false);
      m_plot.setAxisVisible(QwtPlot::xBottom, false);
      break;
  }
}
