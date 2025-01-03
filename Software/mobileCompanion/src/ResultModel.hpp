#ifndef RESULTMODEL_HPP
#define RESULTMODEL_HPP

#include <shared/RingBuffer.hpp>
#include <shared/SignalAnalyzer.hpp>

#include <QAbstractListModel>

class ResultModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum Roles
    {
        SampleIndex = Qt::UserRole + 1,
        TriggerAge,
        Timestamp,

        // detection data
        IsForward,
        SpeedsCount,
        SpeedMarkerAge,
        MedianSpeed,
    };

  public:
    ResultModel(size_t maxSize);

    void add(SignalAnalyzer::Results const& results, size_t sampleIndex);

    size_t getLastSampleIndex() const;
    SignalAnalyzer::Results getLastCompleteResult() const;

  public:
    QHash<int, QByteArray> roleNames() const override;
    int rowCount(QModelIndex const&) const override;
    QVariant data(QModelIndex const& index, int role) const override;

  signals:
    void resultAdded();
    void detectionAdded();

  private:
    struct Item
    {
        size_t sampleIndex = 0;
        size_t triggerAge = 0;
        SignalAnalyzer::Results::ObjectDetection detection;
    };

  private:
    void add(SignalAnalyzer::Results::ObjectDetection const& detection, size_t sampleIndex, size_t triggerAge);

  private:
    RingBuffer<Item> items;
    RingBuffer<size_t> timestamps;
    size_t itemCount = 0;

    size_t lastSampleIndex = -1;
    SignalAnalyzer::Results lastResult;
};

#endif // RESULTMODEL_HPP
