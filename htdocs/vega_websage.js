"use strict";

// OSHMI/Open Substation HMI - Copyright 2008-2017 - Ricardo L. Olsen

WebSAGE.g_vega_num = 0;

WebSAGE.SetIniExtended =
function (inksage_labelvec, lbv, item)
{
  var j;

  // For Vega Specification 2
  function parse(spec)
  {
    if ( typeof(spec.data[0].name) != "undefined" )
      item.vgTableName[0] = spec.data[0].name;
    else
      item.vgTableName[0] = "table";
    var view = vg.parse.spec(spec,
        function (error, chart)
      {
        var svg;

        WebSAGE.g_vega_num++;
        item.vgId = 'vega_' + WebSAGE.g_vega_num;
        $("#VEGACHARTS").append("<div id='" + item.vgId + "'><div>");
        item.vw = chart(
          {
            el : "#vega_" + WebSAGE.g_vega_num,
            renderer : "svg"
          }
          ).update();
        if (typeof(item.vgFormat) != "undefined")
          item.vw._model._defs.data[0].format = item.vgFormat;
        var layer = "layer1";
        var sodipodibase = SVGDoc.querySelector("svg").getElementById("base");
        if (sodipodibase)
          layer = sodipodibase.attributes["inkscape:current-layer"].value;

        //item.vw.width(item.getAttributeNS( null, 'width' )).height(item.getAttributeNS( null, 'height' )).update();
        svg = d3.select(SVGDoc.getElementsByTagName("svg").item(0));
        if (item.parentNode.nodeName == "g")
        {
          item.vg = svg.select("#" + item.parentNode.id).append('g').
            html($("#vega_" + WebSAGE.g_vega_num + " .vega")[0].childNodes[0].childNodes[0].innerHTML);
        }
        else
        {
          item.vg = svg.select("#" + layer).append('g').
            html($("#vega_" + WebSAGE.g_vega_num + " .vega")[0].childNodes[0].childNodes[0].innerHTML);
        }

        item.vg[0][0].id = "vg_" + item.id;

        // position according to the rectangle
        item.vg[0][0].setAttributeNS(null,
          'transform',
          (item.getAttributeNS(null, "transform") || "") +
          " translate(" + item.getAttributeNS(null, "x") + " " +
          item.getAttributeNS(null, "y") + ") "
           +
          " scale(" + item.getAttributeNS(null, 'width') / item.vw._width + " " +
          item.getAttributeNS(null, 'height') / item.vw._height + ") ");

        item.inittransform = item.vg[0][0].getAttributeNS(null, 'transform');
        if ( item.vw.data(item.vgTableName[0]).values().length == 0 )
          item.vgInitData = JSON.parse(JSON.stringify(item.vw._model._data.source._data));
        else
          item.vgInitData = JSON.parse(JSON.stringify(item.vw.data(item.vgTableName[0]).values()));
      }
      );
  };

  // For Vega Specification V3
  function parsev3(spec)
  {
    if ( typeof(spec.data[0].name) != "undefined" )
      item.vgTableName[0] = spec.data[0].name;
    else
      item.vgTableName[0] = "table";
    var view = vega.parse(spec);
    var svg;

    WebSAGE.g_vega_num++;
    item.vgId = 'vega_' + WebSAGE.g_vega_num;
    $("#VEGACHARTS").append("<div id='" + item.vgId + "'><div>");
    item.vw = new vega.View(view, {renderer: "svg"}).
    initialize(/*document.querySelector("#vega_" + WebSAGE.g_vega_num)*/).
    run();    
    item.vw.runAfter(function(){

      var pms = item.vw.toSVG();
      pms.then( function(svgstr) {

        if (typeof(item.vgFormat) != "undefined" && typeof(item.vw._model) != "undefined" )
          item.vw._model._defs.data[0].format = item.vgFormat;
        var layer = "layer1";
        var sodipodibase = SVGDoc.querySelector("svg").getElementById("base");
        if (sodipodibase)
          layer = sodipodibase.attributes["inkscape:current-layer"].value;
  
        svg = d3.select(SVGDoc.getElementsByTagName("svg").item(0));
        if (item.parentNode.nodeName == "g")
        {
          item.vg = svg.select("#" + item.parentNode.id).append('g').
            html( svgstr /*$("#vega_" + WebSAGE.g_vega_num + " .marks")[0].childNodes[0].childNodes[0].innerHTML */);
        }
        else
        {
          item.vg = svg.select("#" + layer).append('g').
            html( svgstr /*$("#vega_" + WebSAGE.g_vega_num + " .marks")[0].childNodes[0].childNodes[0].innerHTML */);
        }
  
        item.vg[0][0].id = "vg_" + item.id;
  
        // position according to the rectangle
        item.vg[0][0].setAttributeNS(null,
          'transform',
          (item.getAttributeNS(null, "transform") || "") +
          " translate(" + item.getAttributeNS(null, "x") + " " +
          item.getAttributeNS(null, "y") + ") "
          +
          " scale(" + item.getAttributeNS(null, 'width') / (item.vw._width || 100) + " " +
          item.getAttributeNS(null, 'height') / (item.vw._height || 100) + ") ");
  
        item.inittransform = item.vg[0][0].getAttributeNS(null, 'transform');
        item.vgInitData = JSON.parse(JSON.stringify(item.vw._runtime.data[item.vgTableName[0]].values.value));
        });
      });

  };

  switch (inksage_labelvec[lbv].tag)
  {
  case "#arc": // arc for donut like graphics
    WebSAGE.acrescentaPontoLista(inksage_labelvec[lbv].src);
    item._d3arc_tag = inksage_labelvec[lbv].src;
    var params = inksage_labelvec[lbv].prompt.split(",");
    item._d3arc_min = parseFloat(params[0] || 0);
    item._d3arc_max = parseFloat(params[1] || 100);
    item._d3arc_innerRadius = parseFloat(params[2] || 0);
    var arc = d3.svg.arc()
                .innerRadius(item._d3arc_innerRadius)
                .outerRadius(100)
                .startAngle(0)
                .endAngle(0);
    var sl = window.d3.select(item.parentNode)
                      .append("path")
                      .style("fill", "red")
                      .attr("d", arc);
    item._d3arc = sl;
    sl[0][0].style.cssText = item.style.cssText;
    var bb = item.getBBox();
    sl[0][0].setAttributeNS( null, 'transform',  item.getAttributeNS( null, 'transform' ) || "" +
           ' translate( ' + (bb.x + bb.width/2) + ',' + (bb.y + bb.height/2) + ' ' + ') ' +
           ' scale( ' + (bb.width/200) + ',' + (bb.height/200) + ' ' + ') ' 
           );

    //sl[0][0].transform = item.transform;
    item.style.display = "none";
    break;

  case "#vega-lite": // vega-lite V1 or V2 chart, defined under a rectangle
    {
      item.vgTableName = [];
      item.style.display = 'none'; // hide the rectangle
      // point list in the Source field
      item.pnts = inksage_labelvec[lbv].src.split(",");
      for (j = 0; j < item.pnts.length; j++)
      {
        WebSAGE.acrescentaPontoLista(item.pnts[j]);
      }
      
      function procvegalite(data)
      {
        var spc;
        if (typeof(data) === 'object')
          {
            spc = vl.compile(data).spec;
            spc['width'] = spc['width'] || 100;
            spc['height'] = spc['height'] || 100;
          }          
        else
          {
            var obj = JSON.parse(data);
            spc = vl.compile(obj).spec;
            spc['width'] = spc['width'] || obj.width || 100;
            spc['height'] = spc['height'] || obj.height || 100;
          }
        if (typeof(spc['data'][0].format) != "undefined")
        {
          item.vgFormat = spc['data'][0].format;
          spc['data'][0].format = {}; // este format faz com que se perca a marcacao PNT#...
        }
        parsev3(spc)
      }

      // test to see if it is a vega definition, if not get as URL
      if ( inksage_labelvec[lbv].prompt.indexOf("{") == 0 )
        procvegalite(inksage_labelvec[lbv].prompt);
      else
        $.get(inksage_labelvec[lbv].prompt + "?" + new Date().getTime(), procvegalite );
    }
    break;
  case "#vega": // vega V2 charts, defined under a rectangle
    {
      item.vgTableName = [];
      item.style.display = 'none'; // hide the rectangle
      // point list in the Source field
      var srcsplit = inksage_labelvec[lbv].src.split("|");
      if (srcsplit.length > 1)
      {
        var paramsplit = srcsplit[1].split(",");
        if (paramsplit.length == 0)
          item.timespan = item.width.baseVal.value;
        else
          item.timespan = paramsplit[0];
      }
      else
        item.timespan = item.width.baseVal.value;

      item.pnts = srcsplit[0].split(",");
      for (j = 0; j < item.pnts.length; j++)
      {
        WebSAGE.acrescentaPontoLista(item.pnts[j]);
      }

      if ( inksage_labelvec[lbv].prompt.indexOf("{") == 0 )
       {
       var spc = JSON.parse(inksage_labelvec[lbv].prompt);
       parse(spc);
       }
      else
      $.get(inksage_labelvec[lbv].prompt + "?" + new Date().getTime(),
        function (data)
        {
          var spc;
          if (typeof(data) === 'object')
            spc = data;
          else
            spc = JSON.parse(data);
          parse(spc);
        }
      );
    }
    break;

  case "#vega-json": // vega V2 charts, updated by JSON file obtained periodically, defined under a rectangle
    { 
      item.vgTableName = [];
      item.style.display = 'none'; // hide the rectangle

      $.get(inksage_labelvec[lbv].prompt + "?" + new Date().getTime(),
        function (data)
      {
        var spc;
        if (typeof(data) === 'object')
          spc = data;
        else
          spc = JSON.parse(data);
        parse(spc);
        
        if ( spc.data.length > 0 )
        if ( typeof( spc.data[0].url ) !== "undefined" )
        {
          if ( typeof( spc.data[0].update_period ) != "undefined" )
          setInterval( 
          function() {
            $.get(spc.data[0].url,
            function (data)
            {
            item.vw.data( spc.data[0].name ).remove( function (d) { return true; } );
            if ( typeof(data) === "string" )
              item.vw.data( spc.data[0].name ).insert(JSON.parse(data));
            else
              item.vw.data( spc.data[0].name ).insert(data);
            item.vw.update();
            item.vg.html($("#" + item.vgId + " .vega")[0].childNodes[0].childNodes[0].innerHTML);
            });
          }, spc.data[0].update_period*1000 );        
        } 

        if ( spc.data.length > 1 )
        if ( typeof( spc.data[1].url ) !== "undefined" )
        {
          if ( typeof( spc.data[1].update_period ) != "undefined" )
          setInterval( 
          function() {
            $.get(spc.data[1].url,
            function (data)
            {
            item.vw.data( spc.data[1].name ).remove( function (d) { return true; } );
            if ( typeof(data) === "string" )
              item.vw.data( spc.data[0].name ).insert(JSON.parse(data));
            else
              item.vw.data( spc.data[0].name ).insert(data);
            item.vw.update();
            item.vg.html($("#" + item.vgId + " .vega")[0].childNodes[0].childNodes[0].innerHTML);
            });
          }, spc.data[1].update_period*1000 );        
        } 
        
      }
      );
    }
    break;

  case "#vega3": // vega V3 charts, defined under a rectangle
    {
      item.vgTableName = [];
      item.style.display = 'none'; // hide the rectangle
      // point list in the Source field
      var srcsplit = inksage_labelvec[lbv].src.split("|");
      if (srcsplit.length > 1)
      {
        var paramsplit = srcsplit[1].split(",");
        if (paramsplit.length == 0)
          item.timespan = item.width.baseVal.value;
        else
          item.timespan = paramsplit[0];
      }
      else
        item.timespan = item.width.baseVal.value;
      item.pnts = srcsplit[0].split(",");
      for (j = 0; j < item.pnts.length; j++)
      {
        WebSAGE.acrescentaPontoLista(item.pnts[j]);
      }

      if ( inksage_labelvec[lbv].prompt.indexOf("{") == 0 )
       {
        var spc = JSON.parse(inksage_labelvec[lbv].prompt);
        spc['width'] = spc['width'] || 100;
        spc['height'] = spc['height'] || 100;
        parsev3(spc);
       }
      else 
      $.get(inksage_labelvec[lbv].prompt + "?" + new Date().getTime(),
          function (data)
        {
          var spc;
          if (typeof(data) === 'object')
            spc = data;
          else
            spc = JSON.parse(data);
          spc['width'] = spc['width'] || 100;
          spc['height'] = spc['height'] || 100;
          parsev3(spc);
        }
      );
    }
    break;

  case "#vega3-json": // vega V3 charts, updated by JSON file obtained periodically, defined under a rectangle
    { 
      item.vgTableName = [];
      item.style.display = 'none'; // hide the rectangle

      $.get(inksage_labelvec[lbv].prompt + "?" + new Date().getTime(),
        function (data)
      {
        var spc;
        if (typeof(data) === 'object')
          spc = data;
        else
          spc = JSON.parse(data);
        parsev3(spc);
        
        if ( spc.data.length > 0 )
        if ( typeof( spc.data[0].url ) !== "undefined" )
        {
          if ( typeof( spc.data[0].update_period ) != "undefined" )
          setInterval( 
          function() {
            $.get(spc.data[0].url,
            function (data)
            {
            if ( typeof(data) === "string" )
              item.vw.change( spc.data[0].name, 
                              vega.changeset().
                              remove( function (d) { return true; } ).
                              insert( JSON.parse(data) )).run();
            else
              item.vw.change( spc.data[0].name, 
                              vega.changeset().
                              remove( function (d) { return true; } ).
                              insert( data )).run();
            item.vw.runAfter(function(){
                    var pms = item.vw.toSVG();
                    pms.then( function(svgstr) {
                      item.vg.html(svgstr);
                      });
                    });              
            });
          }, spc.data[0].update_period*1000 );        
        } 

        if ( spc.data.length > 1 )
        if ( typeof( spc.data[1].url ) !== "undefined" )
        {
          if ( typeof( spc.data[1].update_period ) != "undefined" )
          setInterval( 
          function() {
            $.get(spc.data[1].url,
            function (data)
            {
            if ( typeof(data) === "string" )
              item.vw.change( spc.data[1].name, 
                              vega.changeset().
                              remove( function (d) { return true; } ).
                              insert( JSON.parse(data) )).run();
            else
              item.vw.change( spc.data[1].name, 
                              vega.changeset().
                              remove( function (d) { return true; } ).
                              insert( data )).run();
            item.vw.runAfter(function(){
              var pms = item.vw.toSVG();
              pms.then( function(svgstr) {
                item.vg.html(svgstr);
                });
              }); 
            });
          }, spc.data[1].update_period*1000 );        
        } 
        
      }
      );
    }
    break;

    /*
    case "#multibar": //
    item.style.display = 'none'; // hide the rectangle
    d3.json('multiBarHorizontalData.json', function(data) {
    nv.addGraph(function() {
    var chart = nv.models.multiBarHorizontalChart()
    .x(function(d) { return d.label })
    .y(function(d) { return d.value })
    .width(item.getAttributeNS( null, 'width' ))
    .height(item.getAttributeNS( null, 'height' ))
    .margin({top: 30, right: 20, bottom: 50, left: 175})
    .showValues(true)           //Show bar value next to each bar.
    .color( d3.scale.ordinal().range( [ item.style.fill, item.style.stroke ] ))
    //.tooltip(true)             //Show tooltips on hover.
    //.transitionDuration(350)
    .showControls(false)        //Allow user to switch between "Grouped" and "Stacked" mode.;
    chart.yAxis
    .tickFormat(d3.format(',.2f'));

    var svg = d3.select( SVGDoc.getElementsByTagName("svg").item(0) );

    item.mb = svg.select("#layer1").append('g').
    style("font-size", "14").
    style("font-family", "consolas,Trebuchet MS,arial").
    datum(data).call( chart );

    // position according to the rectangle
    item.mb[0][0].setAttributeNS( null,
    'transform',
    ( item.getAttributeNS( null, "transform" ) || "" )  +
    " translate(" + item.getAttributeNS( null, "x" ) + "," +
    item.getAttributeNS( null, "y" ) + ") " );

    // nv.utils.windowResize(chart.update);

    return chart;
    });
    });
    break;
     */
  }
};

WebSAGE.SetExeExtended =
function (i)
{
  var j;
  switch (WebSAGE.InkSage[i].tag)
  {
  case "#arc":
    var vt = WebSAGE.valorTagueado(WebSAGE.InkSage[i].parent._d3arc_tag);
    var proporcao = ( vt - WebSAGE.InkSage[i].parent._d3arc_min ) / ( WebSAGE.InkSage[i].parent._d3arc_max - WebSAGE.InkSage[i].parent._d3arc_min );
    var arc = d3.svg.arc()
                .innerRadius(WebSAGE.InkSage[i].parent._d3arc_innerRadius)
                .outerRadius(100)
                .startAngle(0)
                .endAngle( proporcao * 2 * Math.PI );    
    WebSAGE.InkSage[i].parent._d3arc.attr("d", arc);
    break;
  case "#vega": // vega V2 chart, defined under a rectangle

    if (typeof(WebSAGE.InkSage[i].parent.vw) != "undefined")
    {
      WebSAGE.InkSage[i].parent.vw.data(WebSAGE.InkSage[i].parent.vgTableName[0]).remove( function (d) { return true; } );

      // copia dados iniciais para o novo
      var newdata = JSON.parse(JSON.stringify(WebSAGE.InkSage[i].parent.vgInitData));
    }

    // procura "PNT#*", etc. onde * eh a ordem do ponto, substitui pelo valor do ponto linkado em SAGE/source
    var d = new Date();
    var cnt_his = 0;
    if (WebSAGE.InkSage[i].parent.hasOwnProperty('vgInitData'))
      $.each(WebSAGE.InkSage[i].parent.vgInitData, function (index, value)
      {
        $.each(value, function (ix, vl)
        {
          if (typeof(vl) === "string")
          {
            if (vl.indexOf("HIS#") >= 0)
            {
              cnt_his++;
              var pnt = WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1];

              if (cnt_his === 1)
                newdata = [];

              if (WebSAGE.InkSage[i].hasOwnProperty('valores') &&
                typeof(WebSAGE.InkSage[i].valores[pnt]) != "undefined")
              {
                for (j = 0; j < WebSAGE.InkSage[i].valores[pnt].length; j++)
                {
                  newdata.push(
                  {
                    "pnt" : "" + pnt,
                    "x" : WebSAGE.InkSage[i].datas[pnt][j],
                    "y" : WebSAGE.InkSage[i].valores[pnt][j]
                  }
                  );
                }

                // bota no historico do ponto o valor novo e tira o antigo
                WebSAGE.InkSage[i].valores[pnt].push(V[pnt]);
                WebSAGE.InkSage[i].datas[pnt].push(d.getTime());
                for (var indv = WebSAGE.InkSage[i].valores[pnt].length - 1; indv >= 0; indv--)
                {
                  var secdif = (d.getTime() - WebSAGE.InkSage[i].datas[pnt][indv]) / 1000;
                  if (secdif > WebSAGE.InkSage[i].parent.timespan * 60)
                  { // o tempo ficou muito grande, tira da lista
                    WebSAGE.InkSage[i].valores[pnt].splice(indv, 1);
                    WebSAGE.InkSage[i].datas[pnt].splice(indv, 1);
                  }
                }
              }
              else
              {
                // call server to get historic data to fill plot, control to call only one time per obj per point
                var utm = (new Date()).getTime() - WebSAGE.InkSage[i].parent.timespan * 60 * 1000;
                if (!WebSAGE.InkSage[i].parent.hasOwnProperty("histCalls"))
                {
                  WebSAGE.InkSage[i].parent.histCalls = [];
                }
                if (!WebSAGE.InkSage[i].parent.histCalls.hasOwnProperty(pnt))
                {
                  var calltps = '$.getScript( WebSAGE.g_timePntServer + "?P=' + pnt + '&U=' + utm / 1000 + '&F=S' + '&B=histdata(' + i + ',' + pnt + ');histdata' + '"' + ' );';
                  WebSAGE.g_timeshift = WebSAGE.g_timeshift + 2000;
                  setTimeout(calltps, WebSAGE.g_timeshift);
                  WebSAGE.InkSage[i].parent.histCalls[pnt] = true;
                }
              }
            }

            if (vl.indexOf("PNT#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.valorTagueado(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                else
                  newdata.splice(-1, 1);
              }
            else
            if (vl.indexOf("FLG#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getFlags(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
            else
            if (vl.indexOf("FLR#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getFlags(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]) & 0x80 ? 1 : 0;
              }
            else
            if (vl.indexOf("LMI#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getInfLim(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
            else
            if (vl.indexOf("LMS#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getSupLim(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
            else
            if (vl.indexOf("DCR#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getDescription(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
            else
            if (vl.indexOf("BAY#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getBay(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
            else
            if (vl.indexOf("SUB#") >= 0)
              {
                if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                  newdata[index][ix] = WebSAGE.getSubstation(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
              }
          }
        }
        );
      }
      );
    if (typeof(WebSAGE.InkSage[i].parent.vw) != "undefined")
    {
      WebSAGE.InkSage[i].parent.vw.data(WebSAGE.InkSage[i].parent.vgTableName[0]).insert(newdata);
      WebSAGE.InkSage[i].parent.vw.update();
      WebSAGE.InkSage[i].parent.vg.html($("#" + WebSAGE.InkSage[i].parent.vgId + " .vega")[0].childNodes[0].childNodes[0].innerHTML);

      // animate as that:
      //d3.select( SVGDoc.getElementById(WebSAGE.InkSage[i].parent.vg[0][0].id)  ).
      //   transition().
      //   duration(1250).
      //   attr("transform", "translate( " + WebSAGE.InkSage[i].parent.vg[0][0].getBBox().width/2 + ",0) " + WebSAGE.InkSage[i].parent.inittransform + " scale(0, 1)");
    }
    break;
  case "#vega-lite": // vega-lite V1 or V2 chart, defined under a rectangle
  case "#vega3": // vega V3 chart, defined under a rectangle
  
      if (typeof(WebSAGE.InkSage[i].parent.vw) != "undefined")
      {
        //WebSAGE.InkSage[i].parent.vw.change(WebSAGE.InkSage[i].parent.vgTableName[0], 
        //                                    vega.changeset().remove( function (d) { return true; } )).run();        

        // copia dados iniciais para o novo
        var newdata = JSON.parse(JSON.stringify(WebSAGE.InkSage[i].parent.vgInitData));
      }
  
      // procura "PNT#*", etc. onde * eh a ordem do ponto, substitui pelo valor do ponto linkado em SAGE/source
      var d = new Date();
      var cnt_his = 0;
      if (WebSAGE.InkSage[i].parent.hasOwnProperty('vgInitData'))
        $.each(WebSAGE.InkSage[i].parent.vgInitData, function (index, value)
        {
          $.each(value, function (ix, vl)
          {
            if (typeof(vl) === "string")
            {
              if (vl.indexOf("HIS#") >= 0)
              {
                cnt_his++;
                var pnt = WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1];
  
                if (cnt_his === 1)
                  newdata = [];
  
                if (WebSAGE.InkSage[i].hasOwnProperty('valores') &&
                  typeof(WebSAGE.InkSage[i].valores[pnt]) != "undefined")
                {
                  for (j = 0; j < WebSAGE.InkSage[i].valores[pnt].length; j++)
                  {
                    newdata.push(
                    {
                      "pnt" : "" + pnt,
                      "x" : WebSAGE.InkSage[i].datas[pnt][j],
                      "y" : WebSAGE.InkSage[i].valores[pnt][j]
                    }
                    );
                  }
  
                  // bota no historico do ponto o valor novo e tira o antigo
                  WebSAGE.InkSage[i].valores[pnt].push(V[pnt]);
                  WebSAGE.InkSage[i].datas[pnt].push(d.getTime());
                  for (var indv = WebSAGE.InkSage[i].valores[pnt].length - 1; indv >= 0; indv--)
                  {
                    var secdif = (d.getTime() - WebSAGE.InkSage[i].datas[pnt][indv]) / 1000;
                    if (secdif > WebSAGE.InkSage[i].parent.timespan * 60)
                    { // o tempo ficou muito grande, tira da lista
                      WebSAGE.InkSage[i].valores[pnt].splice(indv, 1);
                      WebSAGE.InkSage[i].datas[pnt].splice(indv, 1);
                    }
                  }
                }
                else
                {
                  // call server to get historic data to fill plot, control to call only one time per obj per point
                  var utm = (new Date()).getTime() - WebSAGE.InkSage[i].parent.timespan * 60 * 1000;
                  if (!WebSAGE.InkSage[i].parent.hasOwnProperty("histCalls"))
                  {
                    WebSAGE.InkSage[i].parent.histCalls = [];
                  }
                  if (!WebSAGE.InkSage[i].parent.histCalls.hasOwnProperty(pnt))
                  {
                    var calltps = '$.getScript( WebSAGE.g_timePntServer + "?P=' + pnt + '&U=' + utm / 1000 + '&F=S' + '&B=histdata(' + i + ',' + pnt + ');histdata' + '"' + ' );';
                    WebSAGE.g_timeshift = WebSAGE.g_timeshift + 2000;
                    setTimeout(calltps, WebSAGE.g_timeshift);
                    WebSAGE.InkSage[i].parent.histCalls[pnt] = true;
                  }
                }
              }
  
              if (vl.indexOf("PNT#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.valorTagueado(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                  else
                    newdata.splice(-1, 1);
                }
              else
              if (vl.indexOf("FLG#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getFlags(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              else
              if (vl.indexOf("FLR#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getFlags(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]) & 0x80 ? 1 : 0;
                }
              else
              if (vl.indexOf("LMI#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getInfLim(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              else
              if (vl.indexOf("LMS#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getSupLim(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              else
              if (vl.indexOf("DCR#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getDescription(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              else
              if (vl.indexOf("BAY#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getBay(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              else
              if (vl.indexOf("SUB#") >= 0)
                {
                  if (WebSAGE.InkSage[i].parent.pnts.length > vl.split("#")[1] - 1) // testa se existem mais pontos no arquivo json que ponto linkados em SAGE/source
                    newdata[index][ix] = WebSAGE.getSubstation(WebSAGE.InkSage[i].parent.pnts[vl.split("#")[1] - 1]);
                }
              }
          }
          );
        }
        );
      if (typeof(WebSAGE.InkSage[i].parent.vw) != "undefined")
      {
        WebSAGE.InkSage[i].parent.vw.change(WebSAGE.InkSage[i].parent.vgTableName[0], 
                                            vega.changeset().
                                            remove( function (d) { return true; } ).
                                            insert( newdata )).run();
        
        WebSAGE.InkSage[i].parent.vw.runAfter( function(){          
                var pms = WebSAGE.InkSage[i].parent.vw.toSVG();
                pms.then( function(svgstr) {
                  WebSAGE.InkSage[i].parent.vg.html(svgstr);
                });
        });        

      }
      break;    
  }
};

WebSAGE.alternate = function (period, id1, id2)
{
  WebSAGE.idalt1 = id1;
  WebSAGE.idalt2 = id2;
  window.setTimeout(function (id)
  {
    window.WebSAGE.flipOut(id);
  }, 500, id2);
  window.setInterval(function fn(a, b)
  {
    // use fn to create static variables
    var tmp = fn.ida || a;
    fn.ida = fn.idb || b;
    fn.idb = tmp;
    window.WebSAGE.flipOut(fn.idb);
    window.WebSAGE.flipIn(fn.ida);
  }, period, id1, id2);
};

WebSAGE.flipOut = function (id)
{
  var elem = window.SVGDoc.querySelector("svg").getElementById(id);
  if (elem === null)
    return;
  if (typeof(elem.inittransform) === "undefined")
  {
    if (elem.getAttributeNS(null, "transform") === null)
    {
      elem.inittransform = "";
    }
    else
    {
      elem.inittransform = elem.getAttributeNS(null, "transform");
    }
  }
  window.d3.select(elem).
  transition().
  duration(1250).
  attr("transform", "translate( " + elem.getBBox().x + ",0) " + elem.inittransform + " scale(0, 1)");
};

WebSAGE.flipIn = function (id)
{
  var elem = window.SVGDoc.querySelector("svg").getElementById(id);
  if (elem === null)
    return;
  if (typeof(elem.inittransform) === "undefined")
  {
    if (elem.getAttributeNS(null, "transform") === null)
    {
      elem.inittransform = "";
    }
    else
    {
      elem.inittransform = elem.getAttributeNS(null, "transform");
    }
  }
  window.d3.select(elem).
  transition().
  duration(1250).
  attr("transform", /*"translate(0,0) " + */
    elem.inittransform + " scale(1, 1)");
};

if ( typeof(xPlain) !== "undefined" )
  {
  WebSAGE.janelaInfo = function ()  {};
  document.title = gup("SELTELA").toUpperCase().replace("../SVG/", "").replace(".SVG", "") + " - " + Msg.NomeProduto + " - " + Msg.VersaoProduto;
  }
