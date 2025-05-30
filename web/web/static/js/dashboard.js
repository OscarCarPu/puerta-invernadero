let chart = null;

function formatMadridISODate(dateString) {
  const date = new Date(dateString);

  const options = {
    year: 'numeric',
    month: '2-digit',
    day: '2-digit',
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
    timeZone: 'Europe/Madrid',
    hourCycle: 'h23',
  };

  const formatter = new Intl.DateTimeFormat('es-ES', options);
  const parts = formatter.formatToParts(date);
  const year = parts.find((part) => part.type === 'year').value;
  const month =
      parts.find((part) => part.type === 'month').value.padStart(2, '0');
  const day = parts.find((part) => part.type === 'day').value.padStart(2, '0');
  const hour =
      parts.find((part) => part.type === 'hour').value.padStart(2, '0');
  const minute =
      parts.find((part) => part.type === 'minute').value.padStart(2, '0');
  const second =
      parts.find((part) => part.type === 'second').value.padStart(2, '0');
  return `${year}-${month}-${day}T${hour}:${minute}:${second}`;
}

// Función para obtener datos del servidor
async function fetchData() {
  try {
    const response = await fetch('/api/lecturas_recientes');
    const data = await response.json();

    if (data.error) {
      console.error('Error al obtener datos:', data.error);
      return;
    }

    // Actualizar valores actuales
    document.getElementById('currentTemp').textContent =
        data.latest.temperatura ? data.latest.temperatura.toFixed(1) : '--';
    document.getElementById('currentHumidity').textContent =
        data.latest.humedad ? data.latest.humedad.toFixed(1) : '--';

    // Actualizar gráfico
    updateChart(data);

    // Actualizar tabla de lecturas
    updateTable(data);

    // Actualizar hora de última actualización
    document.getElementById('lastUpdate').textContent =
        new Date().toLocaleString('es-ES', {
          timeZone: 'Europe/Madrid',
          year: 'numeric',
          month: '2-digit',
          day: '2-digit',
          hour: '2-digit',
          minute: '2-digit',
          second: '2-digit',
        });
  } catch (error) {
    console.error('Error al obtener datos:', error);
  }
}

// Función para actualizar el gráfico de Plotly
function updateChart(data) {
  const madridDates = data.fechas.map((date) => {
    return formatMadridISODate(date);
  });

  // Calculate initial focus range (last 80 points or 5 minutes)
  let startIndex = 0;
  if (madridDates.length > 0) {
    const lastDate = new Date(madridDates[madridDates.length - 1]);
    const fiveMinutesAgo =
        new Date(lastDate.getTime() - 5 * 60 * 1000);  // 5 minutes ago

    // Find index for 5 minutes ago
    let timeBasedStartIndex = madridDates.findIndex(
        (dateStr) => new Date(dateStr) >= fiveMinutesAgo,
    );
    if (timeBasedStartIndex === -1) timeBasedStartIndex = 0;

    // Use the more restrictive of: last 80 points or 5 minutes
    const pointBasedStartIndex = Math.max(0, madridDates.length - 80);
    startIndex = Math.max(timeBasedStartIndex, pointBasedStartIndex);
  }

  const trace1 = {
    x: madridDates,
    y: data.temperaturas,
    type: 'scatter',
    mode: 'lines+markers',
    name: 'Temperatura (°C)',
    line: {
      color: '#ff6b6b',
      width: 3,
    },
    marker: {
      size: 8,
      color: '#ff6b6b',
    },
    yaxis: 'y',
  };

  const trace2 = {
    x: madridDates,
    y: data.humedades,
    type: 'scatter',
    mode: 'lines+markers',
    name: 'Humedad (%)',
    line: {
      color: '#4ecdc4',
      width: 3,
    },
    marker: {
      size: 8,
      color: '#4ecdc4',
    },
    yaxis: 'y2',
  };

  // Calculate y-axis ranges with 10% buffer
  const tempMin = Math.min(...data.temperaturas);
  const tempMax = Math.max(...data.temperaturas);
  const tempRange = tempMax - tempMin;
  const tempBuffer = tempRange * 0.1;

  const humMin = Math.min(...data.humedades);
  const humMax = Math.max(...data.humedades);
  const humRange = humMax - humMin;
  const humBuffer = humRange * 0.1;

  const layout = {
    title: {
      text: '',
      font: {size: 18},
    },
    xaxis: {
      title: 'Tiempo',
      type: 'date',
      tickformat: '%H:%M:%S',
      gridcolor: '#e9ecef',
      range: madridDates.length > 0 ?
          [madridDates[startIndex], madridDates[madridDates.length - 1]] :
          undefined,
    },
    yaxis: {
      title: 'Temperatura (°C)',
      titlefont: {color: '#ff6b6b'},
      tickfont: {color: '#ff6b6b'},
      gridcolor: '#e9ecef',
      side: 'left',
      range: [tempMin - tempBuffer, tempMax + tempBuffer],
    },
    yaxis2: {
      title: 'Humedad (%)',
      titlefont: {color: '#4ecdc4'},
      tickfont: {color: '#4ecdc4'},
      overlaying: 'y',
      side: 'right',
      gridcolor: '#e9ecef',
      range: [humMin - humBuffer, humMax + humBuffer],
    },
    legend: {
      x: 0,
      y: 1.1,
      orientation: 'h',
    },
    margin: {t: 20, r: 80, b: 60, l: 80},
    plot_bgcolor: '#ffffff',
    paper_bgcolor: '#ffffff',
    font: {family: 'Arial, sans-serif'},
  };

  const config = {
    responsive: true,
    displayModeBar: true,
    modeBarButtonsToRemove: ['lasso2d', 'select2d'],
    displaylogo: false,
    toImageButtonOptions: {
      format: 'png',
      filename: 'sensor_data',
      height: 500,
      width: 700,
      scale: 1,
    },
  };

  if (chart === null) {
    Plotly.newPlot('evolutionChart', [trace1, trace2], layout, config);
    chart = true;
  } else {
    Plotly.react('evolutionChart', [trace1, trace2], layout, config);
  }
}

// Función para actualizar la tabla de lecturas
function updateTable(data) {
  const tableBody = document.getElementById('readingsTable');
  if (!tableBody) return;

  // Crear filas de la tabla con los datos más recientes
  let tableHTML = '';
  const reversedData =
      [...data.fechas].reverse();  // Mostrar más recientes primero
  const reversedTemps = [...data.temperaturas].reverse();
  const reversedHumidities = [...data.humedades].reverse();

  for (let i = 0; i < Math.min(10, reversedData.length); i++) {
    const fecha = new Date(reversedData[i]).toLocaleString('es-ES', {
      timeZone: 'Europe/Madrid',
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
    });
    const temp = reversedTemps[i].toFixed(1);
    const humidity = reversedHumidities[i].toFixed(1);

    tableHTML += `
            <tr>
                <td>${fecha}</td>
                <td>
                    <span class="badge bg-danger rounded-pill">
                        ${temp}°C
                    </span>
                </td>
                <td>
                    <span class="badge bg-info rounded-pill">
                        ${humidity}%
                    </span>
                </td>
            </tr>
        `;
  }

  tableBody.innerHTML = tableHTML;
}

// Cargar datos inicialmente
fetchData();

// Actualizar cada segundo
setInterval(fetchData, 1000);

// Animación de carga inicial
document.addEventListener('DOMContentLoaded', function() {
  const cards = document.querySelectorAll('.metric-card');
  cards.forEach((card, index) => {
    card.style.animationDelay = `${index * 0.2}s`;
    card.style.animation = 'fadeInUp 0.6s ease-out forwards';
  });
});
