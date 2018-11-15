//
//  Plot.swift
//  HWMonitorSMC2
//
//  Created by Vector Sigma on 14/10/2018.
//  Copyright © 2018 vector sigma. All rights reserved.
//

import Cocoa
import QuartzCore
import CorePlot


class PlotView: NSView, CPTPlotDataSource, CPTPlotDelegate {
  public let hostView: CPTGraphHostingView = {
    let host = CPTGraphHostingView(frame: NSRect(x: 0, y: 0, width: 111, height: 17))
    return host
  }()
  var plotSpace : CPTXYPlotSpace? = nil
  var sensor : HWMonitorSensor?
  var plotData : [Double] = [Double]()
  let kFrameRate : Double = 5.0  // frames per second
  let kAlpha     : Double =  0.0 // smoothing constant
  
  let kMaxDataPoints : Int = 302
  let kPlotIdentifier : NSString = "Data Source Plot" as NSString // to changed
  
  var dataTimer : Timer? = nil
  var currentIndex : Int = 0
  
  private var graph : CPTXYGraph? = nil
  
  convenience init(frame frameRect: NSRect, sensor: HWMonitorSensor) {
    self.init(frame:frameRect);
    self.sensor = sensor
    self.wantsLayer = true
    self.configure()
  }
  
  
  override init(frame frameRect: NSRect) {
    super.init(frame: frameRect)
  }
  
  required init?(coder decoder: NSCoder) {
    super.init(coder: decoder)
  }
  
  func configure() {
    // Create graph from theme
    let newGraph = CPTXYGraph(frame: CGRect(x: 0, y: 0, width: 111, height: 17))
    
    //let theme = CPTTheme(named: .darkGradientTheme)
    //newGraph.apply(theme)
    
    newGraph.paddingTop    = 1.0
    newGraph.paddingBottom = 0.0
    newGraph.paddingLeft   = 0.0
    newGraph.paddingRight  = 0.0
    
    newGraph.plotAreaFrame?.paddingTop    = 1.0
    newGraph.plotAreaFrame?.paddingBottom = 0.0
    newGraph.plotAreaFrame?.paddingLeft   = 0.0
    newGraph.plotAreaFrame?.paddingRight  = 0.0
    
    //newGraph.plotAreaFrame?.plotArea?.fill = newGraph.plotAreaFrame?.fill
    newGraph.plotAreaFrame?.fill          = nil
    
    newGraph.plotAreaFrame?.borderLineStyle = nil
    newGraph.plotAreaFrame?.cornerRadius    = 0.0
    newGraph.plotAreaFrame?.masksToBorder   = false
    
    self.hostView.hostedGraph = newGraph;
    self.hostView.wantsLayer = true
    self.addSubview(self.hostView)
    
    
    
    // Setup scatter plot space
    self.plotSpace = newGraph.defaultPlotSpace as? CPTXYPlotSpace
    self.plotSpace?.allowsUserInteraction = false
    self.plotSpace?.xRange = CPTPlotRange(location: NSNumber(value: 1.0), length: NSNumber(value: kMaxDataPoints - 2 ))
    
    self.plotSpace?.globalYRange = nil
    self.plotSpace?.yRange = CPTPlotRange(location: NSNumber(value: 1.0), length: NSNumber(value: 100.0))
    self.plotSpace?.globalYRange = plotSpace?.yRange
    // Axes
    let axisSet = newGraph.axisSet as! CPTXYAxisSet
    if let x = axisSet.xAxis {
      x.labelingPolicy = .automatic
      x.majorIntervalLength   = 0.25
      x.minorTicksPerInterval = 9
      x.orthogonalPosition    = 0
      x.labelingPolicy = .none
    }
    
    if let y = axisSet.yAxis {
      y.labelingPolicy = .automatic
      y.majorIntervalLength   = 0.25
      y.minorTicksPerInterval = 3
      y.orthogonalPosition    = 0
      y.labelingPolicy = .none
      y.axisConstraints = CPTConstraints(lowerOffset: 0.0)
    }
    
    // Create a plot that uses the data source method
    let dataSourceLinePlot = CPTScatterPlot(frame: .zero)
    
    dataSourceLinePlot.identifier = kPlotIdentifier
    dataSourceLinePlot.cachePrecision = .double
    
    if let lineStyle = dataSourceLinePlot.dataLineStyle?.mutableCopy() as? CPTMutableLineStyle {
      lineStyle.lineWidth              = 1.0
      var color : NSColor = NSColor.orange
      if #available(OSX 10.13, *) {
        color = NSColor(named: "GraphLineColor")!
      } else {
        color = (getAppearance().name == .vibrantDark) ? NSColor.green : NSColor.blue
      }
      
      lineStyle.lineColor              = CPTColor.init(nsColor: color)
      dataSourceLinePlot.dataLineStyle = lineStyle
    }
    
    dataSourceLinePlot.dataSource = self
    newGraph.add(dataSourceLinePlot)
    
    self.graph = newGraph
    self.graph?.delegate = self
  }
  
  func newData(value: Double) {
    if (self.sensor == nil) { return }
    DispatchQueue.main.async {
      if let thePlot : CPTPlot = self.graph?.plot(withIdentifier: self.kPlotIdentifier) {
        if self.plotData.count >= self.kMaxDataPoints {
          self.plotData.remove(at: 0)
          thePlot.deleteData(inIndexRange: NSMakeRange(0, 1))
        }
        
        if let plotSpace : CPTXYPlotSpace = self.graph?.defaultPlotSpace as? CPTXYPlotSpace {
          let location = (self.currentIndex >= self.kMaxDataPoints) ? (self.currentIndex - self.kMaxDataPoints + 2) : 0
          
          let oldRange : CPTPlotRange = CPTPlotRange(location: NSNumber(value: location > 0 ? location - 1 : 0),
                                                     length: NSNumber(value: self.kMaxDataPoints - 2))
          
          let newRange : CPTPlotRange = CPTPlotRange(location: NSNumber(value: location),
                                                     length: NSNumber(value: self.kMaxDataPoints - 2))
          
          CPTAnimation.animate(plotSpace, property: "xRange", from: oldRange, to: newRange, duration: CGFloat(1.0 / self.kFrameRate))
          
          self.currentIndex += 1
          
          //self.plotSpace?.yRange = CPTPlotRange(location: NSNumber(value: 0.0), length: NSNumber(value: 100.0 ))
          var conformedVal : Double = value
          switch self.sensor!.sensorType {
          case .percent:            fallthrough
          case .gpuIO_percent:      fallthrough
          case .gpuIO_temp:         fallthrough
          case .intelTemp:          fallthrough
          case .temperature:        fallthrough
          case .hdSmartTemp:        fallthrough
          case .hdSmartLife:        conformedVal = value
            
          case .frequencyOther:     conformedVal = 0
          case .frequencyCPU:       fallthrough
          case .intelCPUFrequency:
            // TB (Tutbo boost) is the Max (100 : TB = x : value). But how get it?? ... from the user!
            conformedVal = (value * 100) / AppSd.cpuFrequencyMax
          case .frequencyGPU:       fallthrough // 1500 MHz max?? (100 : 1500 = x : value)
          case .intelGPUFrequency:  conformedVal = (value * 100) / 1500
          case .intelmWh:           fallthrough
          case .intelJoule:         fallthrough
          case .cpuPowerWatt:       fallthrough
          case .intelWatt:
            conformedVal = (value * 100) / AppSd.cpuTDP
            /*
             We need the TDP and no problem using Intel Power Gadget,
             but what to do if we don't have the it? let the user set this value.\
             */
            self.plotSpace?.yRange = CPTPlotRange(location: NSNumber(value: 0.0), length: NSNumber(value: 35.0 ))
            
          case .voltage:            conformedVal = 0
            
          case .multiplier:         conformedVal = 0
          case .battery:            conformedVal = 0
          case .genericBattery:     conformedVal = 0
          case .usb:                conformedVal = 0
          case .gpuIO_coreClock:    conformedVal = (value * 100) / 1500
          case .gpuIO_FanRPM:       fallthrough
          case .tachometer:         conformedVal = (value * 100) / 5000
          case .gpuIO_Watts:        conformedVal = (value * 100) / 500
          case .memory:
            if self.sensor!.unit == HWUnit.MB {
              conformedVal = (value * 100) / SSMemoryInfo.totalMemory()
            }
          case .gpuIO_RamBytes:     conformedVal = 0
          case .mediaSMARTContenitor: conformedVal = 0
          }
          
          
          //DispatchQueue.main.async {
          if conformedVal > 100 { conformedVal = 100 }
          self.plotData.append(conformedVal)
          thePlot.insertData(at: UInt(self.plotData.count - 1), numberOfRecords: 1)
          //}
        }
      }
    }
  }
  
  func numberOfRecords(for plot: CPTPlot) -> UInt {
    return UInt(self.plotData.count)
  }
  
  func number(for plot: CPTPlot, field fieldEnum: UInt, record idx: UInt) -> Any? {
    return (fieldEnum == UInt(CPTScatterPlotField.X.rawValue) ?
      NSNumber(value: Double(idx) + Double(self.currentIndex) - Double(self.plotData.count)) :
      NSNumber(value: self.plotData[Int(idx)]))
  }
}
