#include "ResultModel.hpp"

#include <optional>

ResultModel::ResultModel(size_t maxSize)
    : items(maxSize)
    , timestamps(maxSize)
{}

void ResultModel::add(SignalAnalyzer::Results const& results, size_t sampleIndex)
{
    lastSampleIndex = sampleIndex;
    lastResult = results;

    timestamps.add(results.timestamp);

    auto getTriggerAge = [this](auto const& detection) -> std::optional<size_t> {
        for(size_t i = 0; i < timestamps.size(); i++)
            if(timestamps.get(i) == detection.timestamp)
                return i + lastResult.setupData.hannWindowN / 2;

        return {};
    };

    if(results.completedForwardDetection.has_value())
    {
        auto triggerAge = getTriggerAge(results.completedForwardDetection.value());
        if(triggerAge.has_value())
            add(results.completedForwardDetection.value(), sampleIndex, triggerAge.value());
    }
    if(results.completedReverseDetection.has_value())
    {
        auto triggerAge = getTriggerAge(results.completedReverseDetection.value());
        if(triggerAge.has_value())
            add(results.completedReverseDetection.value(), sampleIndex, triggerAge.value());
    }

    emit resultAdded();
}

size_t ResultModel::getLastSampleIndex() const
{
    return lastSampleIndex;
}
SignalAnalyzer::Results ResultModel::getLastCompleteResult() const
{
    return lastResult;
}

void ResultModel::add(SignalAnalyzer::Results::ObjectDetection const& detection, size_t sampleIndex, size_t triggerAge)
{
    if(bool stillSpaceLeft = itemCount < items.size(); stillSpaceLeft)
    {
        beginInsertRows(QModelIndex(), itemCount, itemCount);
        items.add(Item{sampleIndex, triggerAge, detection});
        itemCount++;
        endInsertRows();
        emit detectionAdded();
        return;
    }

    // we reached our limit
    // fake row removal
    beginRemoveRows(QModelIndex(), 0, 0);
    endRemoveRows();

    beginInsertRows(QModelIndex(), items.size() - 1, items.size() - 1);
    items.add(Item{sampleIndex, triggerAge, detection});
    endInsertRows();
    emit detectionAdded();
}

QHash<int, QByteArray> ResultModel::ResultModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[SampleIndex] = "sampleIndex";
    roles[TriggerAge] = "triggerAge";
    roles[Timestamp] = "timestamp";
    roles[IsForward] = "isForward";
    roles[SpeedsCount] = "speedsCount";
    roles[SpeedMarkerAge] = "speedMarkerAge";
    roles[MedianSpeed] = "medianSpeed";

    return roles;
}

int ResultModel::rowCount(QModelIndex const&) const
{
    return itemCount;
}

QVariant ResultModel::data(QModelIndex const& index, int role) const
{
    if(index.row() < 0 || index.row() >= static_cast<int>(itemCount))
        return QVariant::Invalid;

    size_t rawRow = static_cast<size_t>(index.row());
    auto const& item = items.get(itemCount - rawRow);
    auto const& detection = item.detection;

    switch(role)
    {
        case Roles::SampleIndex:
            return static_cast<unsigned long long>(item.sampleIndex);
        case Roles::TriggerAge:
            return static_cast<unsigned long long>(item.triggerAge);
        case Roles::Timestamp:
            return static_cast<unsigned long long>(detection.timestamp);
        case Roles::IsForward:
            return detection.isForward;
        case Roles::SpeedsCount:
            return static_cast<unsigned long long>(detection.speeds.size());
        case Roles::SpeedMarkerAge:
            return static_cast<unsigned long long>(detection.triggerSignalAge);
        case Roles::MedianSpeed:
            return detection.medianSpeed;
    }

    return QVariant::Invalid;
}
