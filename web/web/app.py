from datetime import datetime, timedelta

import pytz
from flask import Flask, jsonify, render_template, request
from utils import supabase

app = Flask("invernadero")

# Define Madrid timezone
MADRID_TZ = pytz.timezone("Europe/Madrid")


def get_recent_readings():
    """Common function to get recent readings from database"""
    try:
        # Obtener lecturas de la última hora usando timezone de Madrid
        madrid_now = datetime.now(MADRID_TZ)
        one_hour_ago = (madrid_now - timedelta(hours=1)).isoformat()

        response = (
            supabase.table("lectura")
            .select("*")
            .gte("fecha_hora", one_hour_ago)
            .order("fecha_hora", desc=False)
            .execute()
        )

        # Check for Supabase errors
        if hasattr(response, "error") and response.error:
            return None, f"Error en la base de datos: {response.error.message}"

        readings = response.data if hasattr(response, "data") else []

        # Handle empty readings gracefully
        if not readings:
            formatted_data = {
                "temperaturas": [],
                "humedades": [],
                "fechas": [],
                "latest": {"temperatura": 0, "humedad": 0},
                "message": "No se encontraron lecturas en la última hora",
            }
            return formatted_data, None

        # Safely format data for frontend with error handling
        try:
            temperaturas = []
            humedades = []
            fechas = []

            for r in readings:
                if r and isinstance(r, dict):
                    # Safely convert temperature
                    temp = r.get("temperatura")
                    if temp is not None:
                        try:
                            temperaturas.append(float(temp))
                        except (ValueError, TypeError):
                            temperaturas.append(0.0)
                    else:
                        temperaturas.append(0.0)

                    # Safely convert humidity
                    hum = r.get("humedad")
                    if hum is not None:
                        try:
                            humedades.append(float(hum))
                        except (ValueError, TypeError):
                            humedades.append(0.0)
                    else:
                        humedades.append(0.0)

                    # Safely get date
                    fecha = r.get("fecha_hora", "")
                    fechas.append(fecha)

            # Get latest reading safely
            latest_temp = 0.0
            latest_hum = 0.0
            if readings:
                last_reading = readings[-1]
                if last_reading and isinstance(last_reading, dict):
                    temp = last_reading.get("temperatura")
                    hum = last_reading.get("humedad")
                    try:
                        latest_temp = float(temp) if temp is not None else 0.0
                        latest_hum = float(hum) if hum is not None else 0.0
                    except (ValueError, TypeError):
                        latest_temp = 0.0
                        latest_hum = 0.0

            data = {
                "temperaturas": temperaturas,
                "humedades": humedades,
                "fechas": fechas,
                "latest": {"temperatura": latest_temp, "humedad": latest_hum},
            }

            return data, None

        except Exception as format_error:
            return None, f"Error al formatear datos: {str(format_error)}"

    except Exception as e:
        return None, f"Error al obtener lecturas recientes: {str(e)}"


@app.route("/")
def index():
    """Página principal que muestra lecturas de la última hora"""
    data, error = get_recent_readings()

    if error:
        return render_template("error.html", error=error), 500

    return render_template("dashboard.html", data=data)


@app.route("/api/lecturas_recientes", methods=["GET"])
def api_lecturas_recientes():
    """API endpoint para obtener lecturas recientes (ultima hora)"""
    data, error = get_recent_readings()

    if error:
        return jsonify({"error": error}), 500

    return jsonify(data)


@app.route("/lectura", methods=["POST"])
def lectura():
    """Endpoint para recibir una nueva lectura de temperatura y humedad"""
    try:
        # Check content type
        if not request.is_json:
            return jsonify({"error": "Content-Type debe ser application/json"}), 400

        data = request.get_json()

        # Validate JSON data
        if not data:
            return jsonify({"error": "No se recibieron datos JSON válidos"}), 400

        if not isinstance(data, dict):
            return jsonify({"error": "Los datos deben ser un objeto JSON"}), 400

        # Get and validate temperature
        temperatura = data.get("temperatura")
        humedad = data.get("humedad")

        if temperatura is None:
            return jsonify({"error": "Falta el campo 'temperatura'"}), 400

        if humedad is None:
            return jsonify({"error": "Falta el campo 'humedad'"}), 400

        # Convert to float with proper error handling
        try:
            temperatura_float = float(temperatura)
            humedad_float = float(humedad)
        except (ValueError, TypeError):
            return jsonify({"error": "Temperatura y humedad deben ser números válidos"}), 400

        # Validate ranges (optional but recommended)
        if not (-50 <= temperatura_float <= 100):
            return jsonify({"error": "Temperatura fuera del rango válido (-50°C a 100°C)"}), 400

        if not (0 <= humedad_float <= 100):
            return jsonify({"error": "Humedad fuera del rango válido (0% a 100%)"}), 400

        # Insert into database
        try:
            response = (
                supabase.table("lectura")
                .insert({"temperatura": temperatura_float, "humedad": humedad_float})
                .execute()
            )

            # Check for Supabase errors
            if hasattr(response, "error") and response.error:
                return (
                    jsonify({"error": f"Error en base de datos: {response.error.message}"}),
                    500,
                )

            # Check if data was inserted successfully
            if response.data and len(response.data) > 0:
                return (
                    jsonify({"message": "Lectura guardada exitosamente", "data": response.data[0]}),
                    201,
                )
            else:
                return jsonify({"error": "No se pudo guardar la lectura"}), 500

        except Exception as db_error:
            return jsonify({"error": f"Error al guardar en la base de datos: {str(db_error)}"}), 500

    except Exception as e:
        return jsonify({"error": f"Error interno del servidor: {str(e)}"}), 500


if __name__ == "__main__":
    app.run(debug=True)
