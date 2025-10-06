import javax.swing.*;
import java.awt.*;
import java.io.*;
import java.net.*;

public class ObserverClientGUI extends JFrame {
    private static final String SERVER_IP = "127.0.0.1";
    private static final int SERVER_PORT = 8080;
    
    private Socket socket;
    private PrintWriter out;
    private BufferedReader in;
    private Thread listenerThread;
    private volatile boolean running;
    
    // Componentes UI
    private JTextArea logArea;
    private JButton disconnectBtn;
    
    // Labels de telemetría
    private JLabel speedLabel;
    private JLabel batteryLabel;
    private JLabel temperatureLabel;
    private JLabel directionLabel;
    private JLabel statusLabel;
    private JLabel connectionLabel;
    
    public ObserverClientGUI() {
        setTitle("Vehículo Autónomo - Cliente Observador (Java)");
        setSize(600, 550);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLayout(new BorderLayout(10, 10));
        
        initComponents();
        setLocationRelativeTo(null);
        
        // Conectar automáticamente al inicio
        SwingUtilities.invokeLater(() -> connectToServer());
    }
    
    private void initComponents() {
        // Panel superior - Estado de conexión
        JPanel topPanel = new JPanel(new BorderLayout());
        topPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        connectionLabel = new JLabel("Estado: Desconectado");
        connectionLabel.setFont(new Font("Arial", Font.BOLD, 14));
        connectionLabel.setForeground(Color.RED);
        
        disconnectBtn = new JButton("Desconectar");
        disconnectBtn.setBackground(new Color(231, 76, 60));
        disconnectBtn.setForeground(Color.WHITE);
        disconnectBtn.setEnabled(false);
        disconnectBtn.addActionListener(e -> disconnectFromServer());
        
        topPanel.add(connectionLabel, BorderLayout.WEST);
        topPanel.add(disconnectBtn, BorderLayout.EAST);
        
        add(topPanel, BorderLayout.NORTH);
        
        // Panel central - Telemetría
        JPanel centerPanel = new JPanel(new BorderLayout(10, 10));
        centerPanel.setBorder(BorderFactory.createEmptyBorder(10, 10, 10, 10));
        
        JPanel telemetryPanel = new JPanel();
        telemetryPanel.setLayout(new BoxLayout(telemetryPanel, BoxLayout.Y_AXIS));
        telemetryPanel.setBorder(BorderFactory.createTitledBorder(
            BorderFactory.createLineBorder(new Color(52, 152, 219), 2),
            "📡 Telemetría en Tiempo Real",
            0,
            0,
            new Font("Arial", Font.BOLD, 14),
            new Color(52, 152, 219)
        ));
        
        Font labelFont = new Font("Monospaced", Font.BOLD, 16);
        Color labelColor = new Color(44, 62, 80);
        
        speedLabel = createTelemetryLabel("🚗 Velocidad: -- km/h", labelFont, labelColor);
        batteryLabel = createTelemetryLabel("🔋 Batería: --%", labelFont, labelColor);
        temperatureLabel = createTelemetryLabel("🌡️ Temperatura: -- °C", labelFont, labelColor);
        directionLabel = createTelemetryLabel("🧭 Dirección: --", labelFont, labelColor);
        statusLabel = createTelemetryLabel("⚡ Estado: Detenido", labelFont, labelColor);
        
        telemetryPanel.add(Box.createVerticalStrut(10));
        telemetryPanel.add(speedLabel);
        telemetryPanel.add(Box.createVerticalStrut(10));
        telemetryPanel.add(batteryLabel);
        telemetryPanel.add(Box.createVerticalStrut(10));
        telemetryPanel.add(temperatureLabel);
        telemetryPanel.add(Box.createVerticalStrut(10));
        telemetryPanel.add(directionLabel);
        telemetryPanel.add(Box.createVerticalStrut(10));
        telemetryPanel.add(statusLabel);
        telemetryPanel.add(Box.createVerticalStrut(10));
        
        centerPanel.add(telemetryPanel, BorderLayout.CENTER);
        
        add(centerPanel, BorderLayout.CENTER);
        
        // Panel inferior - Log
        JPanel bottomPanel = new JPanel(new BorderLayout());
        bottomPanel.setBorder(BorderFactory.createTitledBorder("Log de Eventos"));
        
        logArea = new JTextArea(8, 50);
        logArea.setEditable(false);
        logArea.setFont(new Font("Monospaced", Font.PLAIN, 11));
        logArea.setBackground(new Color(248, 249, 250));
        JScrollPane scrollPane = new JScrollPane(logArea);
        
        bottomPanel.add(scrollPane, BorderLayout.CENTER);
        
        add(bottomPanel, BorderLayout.SOUTH);
    }
    
    private JLabel createTelemetryLabel(String text, Font font, Color color) {
        JLabel label = new JLabel(text);
        label.setFont(font);
        label.setForeground(color);
        label.setAlignmentX(Component.LEFT_ALIGNMENT);
        return label;
    }
    
    private void logOutput(String text) {
        SwingUtilities.invokeLater(() -> {
            logArea.append(text + "\n");
            logArea.setCaretPosition(logArea.getDocument().getLength());
        });
    }
    
    private void updateConnectionStatus(boolean connected) {
        SwingUtilities.invokeLater(() -> {
            if (connected) {
                connectionLabel.setText("Estado: ✅ Conectado");
                connectionLabel.setForeground(new Color(46, 204, 113));
                disconnectBtn.setEnabled(true);
            } else {
                connectionLabel.setText("Estado: ❌ Desconectado");
                connectionLabel.setForeground(Color.RED);
                disconnectBtn.setEnabled(false);
            }
        });
    }
    
    private void connectToServer() {
        try {
            socket = new Socket(SERVER_IP, SERVER_PORT);
            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            running = true;
            
            logOutput("✅ Conectado al servidor " + SERVER_IP + ":" + SERVER_PORT);
            updateConnectionStatus(true);
            
            // Enviar CONNECT como OBSERVER
            String connectMsg = "VATP/1.0 CONNECT 0\r\n" +
                               "User-Type: OBSERVER\r\n" +
                               "Username: observer\r\n" +
                               "\r\n";
            
            logOutput(">>> Conectando como OBSERVER...");
            out.print(connectMsg);
            out.flush();
            
            // Leer respuesta inicial
            readResponse();
            
            // Iniciar hilo para escuchar telemetría
            startListeningForTelemetry();
            
        } catch (IOException e) {
            logOutput("❌ Error al conectar: " + e.getMessage());
            updateConnectionStatus(false);
            JOptionPane.showMessageDialog(this, 
                "No se pudo conectar al servidor.\n" +
                "Verifique que el servidor esté ejecutándose en " + SERVER_IP + ":" + SERVER_PORT,
                "Error de Conexión", 
                JOptionPane.ERROR_MESSAGE);
        }
    }
    
    private void readResponse() throws IOException {
        StringBuilder response = new StringBuilder();
        String line;
        
        while ((line = in.readLine()) != null) {
            response.append(line).append("\n");
            if (line.trim().isEmpty()) {
                break;
            }
        }
        
        if (response.length() > 0) {
            logOutput("<<< " + response.toString().replace("\n", " ").trim());
        }
    }
    
    private void startListeningForTelemetry() {
        listenerThread = new Thread(() -> {
            try {
                String line;
                StringBuilder buffer = new StringBuilder();
                
                logOutput("📡 Escuchando telemetría del servidor...");
                
                while (running && socket != null && !socket.isClosed()) {
                    line = in.readLine();
                    
                    if (line == null) {
                        logOutput("⚠️ Conexión cerrada por el servidor");
                        break;
                    }
                    
                    buffer.append(line).append("\n");
                    
                    // Detectar fin de mensaje (línea vacía)
                    if (line.trim().isEmpty() && buffer.length() > 0) {
                        String message = buffer.toString();
                        
                        if (message.contains("TELEMETRY_DATA")) {
                            parseTelemetry(message);
                        } else if (message.contains("RESPONSE_OK")) {
                            logOutput("✅ Respuesta del servidor recibida");
                        }
                        
                        buffer.setLength(0);
                    }
                }
            } catch (IOException e) {
                if (running) {
                    logOutput("⚠️ Error en conexión: " + e.getMessage());
                }
            } finally {
                SwingUtilities.invokeLater(() -> {
                    updateConnectionStatus(false);
                    logOutput("🔌 Desconectado del servidor");
                });
            }
        });
        
        listenerThread.setDaemon(true);
        listenerThread.start();
    }
    
    private void parseTelemetry(String message) {
        SwingUtilities.invokeLater(() -> {
            try {
                String[] lines = message.split("\n");
                
                for (String line : lines) {
                    line = line.trim();
                    
                    if (line.startsWith("Speed:")) {
                        String speed = extractValue(line);
                        speedLabel.setText("🚗 Velocidad: " + speed);
                    } else if (line.startsWith("Battery:")) {
                        String battery = extractValue(line);
                        batteryLabel.setText("🔋 Batería: " + battery);
                        
                        // Cambiar color si batería baja
                        try {
                            double batteryValue = Double.parseDouble(battery.replace("%", ""));
                            if (batteryValue < 20) {
                                batteryLabel.setForeground(Color.RED);
                            } else if (batteryValue < 50) {
                                batteryLabel.setForeground(new Color(255, 165, 0));
                            } else {
                                batteryLabel.setForeground(new Color(44, 62, 80));
                            }
                        } catch (NumberFormatException e) {
                            // Ignorar error de parseo
                        }
                    } else if (line.startsWith("Temperature:")) {
                        String temp = extractValue(line);
                        temperatureLabel.setText("🌡️ Temperatura: " + temp);
                    } else if (line.startsWith("Direction:")) {
                        String dir = extractValue(line);
                        directionLabel.setText("🧭 Dirección: " + dir);
                    } else if (line.startsWith("Moving:")) {
                        String moving = extractValue(line);
                        boolean isMoving = moving.equalsIgnoreCase("Yes");
                        statusLabel.setText("⚡ Estado: " + (isMoving ? "En Movimiento" : "Detenido"));
                        statusLabel.setForeground(isMoving ? new Color(46, 204, 113) : new Color(149, 165, 166));
                    }
                }
                
                logOutput("📊 Telemetría actualizada");
                
            } catch (Exception e) {
                logOutput("⚠️ Error parseando telemetría: " + e.getMessage());
            }
        });
    }
    
    private String extractValue(String line) {
        int colonIndex = line.indexOf(":");
        if (colonIndex >= 0 && colonIndex < line.length() - 1) {
            return line.substring(colonIndex + 1).trim();
        }
        return "--";
    }
    
    private void disconnectFromServer() {
        running = false;
        
        try {
            if (socket != null && !socket.isClosed()) {
                String disconnectMsg = "VATP/1.0 DISCONNECT 0\r\n" +
                                      "Username: observer\r\n" +
                                      "\r\n";
                
                out.print(disconnectMsg);
                out.flush();
                
                logOutput(">>> Desconectando...");
                
                Thread.sleep(100); // Dar tiempo para enviar el mensaje
                
                socket.close();
            }
        } catch (Exception e) {
            logOutput("⚠️ Error al desconectar: " + e.getMessage());
        } finally {
            socket = null;
            out = null;
            in = null;
            
            updateConnectionStatus(false);
            
            speedLabel.setText("🚗 Velocidad: -- km/h");
            batteryLabel.setText("🔋 Batería: --%");
            batteryLabel.setForeground(new Color(44, 62, 80));
            temperatureLabel.setText("🌡️ Temperatura: -- °C");
            directionLabel.setText("🧭 Dirección: --");
            statusLabel.setText("⚡ Estado: Detenido");
            statusLabel.setForeground(new Color(44, 62, 80));
            
            logOutput("⏹️ Cliente desconectado correctamente");
        }
    }
    
    public static void main(String[] args) {
        // Configurar el Look and Feel del sistema
        try {
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            // Usar el Look and Feel por defecto si falla
        }
        
        SwingUtilities.invokeLater(() -> {
            ObserverClientGUI gui = new ObserverClientGUI();
            gui.setVisible(true);
        });
    }
}