document.addEventListener('DOMContentLoaded', function () {
    const pieChartCanvas = document.getElementById('pieChart').getContext('2d');
    const barChartCanvas = document.getElementById('barChart').getContext('2d');
    const lineChartCanvas = document.getElementById('lineChart').getContext('2d');
    let pieChartInstance;
    let barChartInstance;
    let lineChartInstance;

    function updatePieChart(study_data) {
        if (pieChartInstance) {
            pieChartInstance.destroy();
        }

        const labels = study_data.map(entry => entry.subject);
        const times = study_data.map(entry => entry.time_minutes);
        const colors = study_data.map(entry => entry.color);

        pieChartInstance = new Chart(pieChartCanvas, {
            type: 'pie',
            data: {
                labels: labels,
                datasets: [{
                    label: '勉強時間',
                    data: times,
                    backgroundColor: colors
                }]
            }
        });
    }

    function updateBarChart(study_data) {
        if (barChartInstance) {
            barChartInstance.destroy();
        }

        const labels = study_data.map(entry => entry.subject);
        const times = study_data.map(entry => entry.time_minutes);
        const colors = study_data.map(entry => entry.color);

        const barData = {
            labels: labels,
            datasets: [{
                label: '勉強時間',
                data: times,
                backgroundColor: colors
            }]
        };

        const options = {
            scales: {
                y: {
                    beginAtZero: true
                }
            }
        };

        barChartInstance = new Chart(barChartCanvas, {
            type: 'bar',
            data: barData,
            options: options
        });
    }

    function updateLineChart(study_data) {
        if (lineChartInstance) {
            lineChartInstance.destroy();
        }

        // 過去7日間の日付の配列を作成
        const dates = [];
        for (let i = 6; i >= 0; i--) {
            const d = new Date();
            d.setDate(d.getDate() - i);
            dates.push(d.toISOString().split('T')[0]);
        }

        // 日付ごとの勉強時間を集計
        const dataByDate = dates.map(date => {
            const studyTime = study_data.reduce((total, entry) => {
                if (entry.date === date) {
                    return total + entry.time_minutes;
                } else {
                    return total;
                }
            }, 0);
            return studyTime;
        });

        lineChartInstance = new Chart(lineChartCanvas, {
            type: 'line',
            data: {
                labels: dates,
                datasets: [{
                    label: '過去7日間の勉強時間',
                    data: dataByDate,
                    borderColor: '#FF6384',
                    fill: false
                }]
            },
            options: {
                scales: {
                    y: {
                        beginAtZero: true
                    }
                }
            }
        });
    }

    function fetchStudyDataAndRenderCharts() {
        fetch('http://localhost:5000/api/study_data')
            .then(response => response.json())
            .then(data => {
                updatePieChart(data);
                updateBarChart(data);
                updateLineChart(data);
            })
            .catch(error => console.error('Error fetching study data:', error));
    }

    document.getElementById('add-button').addEventListener('click', function () {
        const subjectInput = document.getElementById('subject-input');
        const timeInput = document.getElementById('time-input');
        const colorInput = document.getElementById('color-input');

        const subject = subjectInput.value.trim();
        const timeMinutes = parseInt(timeInput.value, 10);
        const color = colorInput.value;

        if (subject !== '' && !isNaN(timeMinutes) && timeMinutes > 0) {
            fetch('http://localhost:5000/api/study_data', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ subject: subject, time_minutes: timeMinutes, color: color })
            })
            .then(response => response.json())
            .then(data => {
                updatePieChart(data);
                updateBarChart(data);
                updateLineChart(data);
                subjectInput.value = '';
                timeInput.value = '';
                colorInput.value = '#4BC0C0';
            })
            .catch(error => console.error('Error adding study data:', error));
        }
    });

    document.getElementById('delete-button').addEventListener('click', function () {
        const subjectInput = document.getElementById('subject-input');
        const timeInput = document.getElementById('time-input');

        const subject = subjectInput.value.trim();
        const timeMinutes = parseInt(timeInput.value, 10);

        if (subject !== '' && !isNaN(timeMinutes) && timeMinutes > 0) {
            fetch(`http://localhost:5000/api/study_data/${encodeURIComponent(subject)}`, {
                method: 'DELETE',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify({ time_minutes: timeMinutes })
            })
            .then(response => response.json())
            .then(data => {
                updatePieChart(data);
                updateBarChart(data);
                updateLineChart(data);
                subjectInput.value = '';
                timeInput.value = '';
            })
            .catch(error => console.error('Error deleting study data:', error));
        }
    });

    fetchStudyDataAndRenderCharts();
});
